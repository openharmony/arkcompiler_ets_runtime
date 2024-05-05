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

constexpr char kClassNameOfMath[] = "Ljava_2Flang_2FMath_3B";
constexpr char kFuncNamePrefixOfMathSqrt[] = "Ljava_2Flang_2FMath_3B_7Csqrt_7C_28D_29D";
constexpr char kFuncNamePrefixOfMathAbs[] = "Ljava_2Flang_2FMath_3B_7Cabs_7C";
constexpr char kFuncNamePrefixOfMathMax[] = "Ljava_2Flang_2FMath_3B_7Cmax_7C";
constexpr char kFuncNamePrefixOfMathMin[] = "Ljava_2Flang_2FMath_3B_7Cmin_7C";
constexpr char kFuncNameOfMathAbs[] = "abs";
constexpr char kFuncNameOfMemset[] = "memset";
constexpr char kFuncNameOfMemcpy[] = "memcpy";
constexpr char kFuncNameOfMemsetS[] = "memset_s";
constexpr char kFuncNameOfMemcpyS[] = "memcpy_s";
constexpr uint64_t kSecurecMemMaxLen = 0x7fffffffUL;
static constexpr int32 kProbUnlikely = 1000;
constexpr uint32_t kMemsetDstOpndIdx = 0;
constexpr uint32_t kMemsetSDstSizeOpndIdx = 1;
constexpr uint32_t kMemsetSSrcOpndIdx = 2;
constexpr uint32_t kMemsetSSrcSizeOpndIdx = 3;

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

static void MayPrintLog(bool debug, bool success, MemOpKind memOpKind, const char *str)
{
    if (!debug) {
        return;
    }
    const char *memop = "";
    if (memOpKind == MEM_OP_memset) {
        memop = "memset";
    } else if (memOpKind == MEM_OP_memcpy) {
        memop = "memcpy";
    } else if (memOpKind == MEM_OP_memset_s) {
        memop = "memset_s";
    } else if (memOpKind == MEM_OP_memcpy_s) {
        memop = "memcpy_s";
    }
    LogInfo::MapleLogger() << memop << " expand " << (success ? "success: " : "failure: ") << str << std::endl;
}

bool Simplify::IsMathSqrt(const std::string funcName)
{
    return (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathSqrt) == 0));
}

bool Simplify::IsMathAbs(const std::string funcName)
{
    return (mirMod.IsCModule() && (strcmp(funcName.c_str(), kFuncNameOfMathAbs) == 0)) ||
           (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathAbs) == 0));
}

bool Simplify::IsMathMax(const std::string funcName)
{
    return (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathMax) == 0));
}

bool Simplify::IsMathMin(const std::string funcName)
{
    return (mirMod.IsJavaModule() && (strcmp(funcName.c_str(), kFuncNamePrefixOfMathMin) == 0));
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
    if (!mirMod.IsCModule() && !mirMod.IsJavaModule()) {
        return false;
    }
    if (mirMod.IsJavaModule() && funcName.find(kClassNameOfMath) == std::string::npos) {
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

StmtNode *Simplify::SimplifyToSelect(MIRFunction *func, IfStmtNode *ifNode, BlockNode *block)
{
    // Example: if (condition) {
    //   Example: res = trueRes
    // Example: }
    // Example: else {
    //   Example: res = falseRes
    // Example: }
    // =================
    // res = select condition ? trueRes : falseRes
    if (ifNode->GetPrev() != nullptr && ifNode->GetPrev()->GetOpCode() == OP_label) {
        // simplify shortCircuit will stop opt in cfg_opt, and generate extra compare
        auto *labelNode = static_cast<LabelNode *>(ifNode->GetPrev());
        const std::string &labelName = func->GetLabelTabItem(labelNode->GetLabelIdx());
        if (labelName.find("shortCircuit") != std::string::npos) {
            return nullptr;
        }
    }
    if (ifNode->GetThenPart() == nullptr || ifNode->GetElsePart() == nullptr) {
        return nullptr;
    }
    StmtNode *thenFirst = ifNode->GetThenPart()->GetFirst();
    StmtNode *elseFirst = ifNode->GetElsePart()->GetFirst();
    if (thenFirst == nullptr || elseFirst == nullptr) {
        return nullptr;
    }
    // thenpart and elsepart has only one stmt
    if (thenFirst->GetNext() != nullptr || elseFirst->GetNext() != nullptr) {
        return nullptr;
    }
    if (thenFirst->GetOpCode() != OP_dassign || elseFirst->GetOpCode() != OP_dassign) {
        return nullptr;
    }
    auto *thenDass = static_cast<DassignNode *>(thenFirst);
    auto *elseDass = static_cast<DassignNode *>(elseFirst);
    if (thenDass->GetStIdx() != elseDass->GetStIdx() || thenDass->GetFieldID() != elseDass->GetFieldID()) {
        return nullptr;
    }
    // iread has sideeffect : may cause deref error
    if (HasIreadExpr(thenDass->GetRHS()) || HasIreadExpr(elseDass->GetRHS())) {
        return nullptr;
    }
    // Check if the operand of the select node is complex enough
    // we should not simplify it to if-then-else for either functionality or performance reason
    if (thenDass->GetRHS()->GetPrimType() == PTY_agg || elseDass->GetRHS()->GetPrimType() == PTY_agg) {
        return nullptr;
    }
    constexpr size_t maxDepth = 3;
    if (MaxDepth(thenDass->GetRHS()) > maxDepth || MaxDepth(elseDass->GetRHS()) > maxDepth) {
        return nullptr;
    }
    if (UseGlobalVar(thenDass->GetRHS()) || UseGlobalVar(elseDass->GetRHS())) {
        return nullptr;
    }
    MIRBuilder *mirBuiler = func->GetModule()->GetMIRBuilder();
    MIRType *type = GlobalTables::GetTypeTable().GetPrimType(thenDass->GetRHS()->GetPrimType());
    auto *selectExpr =
        mirBuiler->CreateExprTernary(OP_select, *type, ifNode->Opnd(0), thenDass->GetRHS(), elseDass->GetRHS());
    auto *newDassign = mirBuiler->CreateStmtDassign(thenDass->GetStIdx(), thenDass->GetFieldID(), selectExpr);
    newDassign->SetSrcPos(ifNode->GetSrcPos());
    block->InsertBefore(ifNode, newDassign);
    block->RemoveStmt(ifNode);
    return newDassign;
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

// Return Fold result expr, does not always return a constant expr
// Attention: Fold may modify the input expr, if foldExpr is not a nullptr, we should always replace expr with foldExpr
static BaseNode *FoldIntConst(BaseNode *expr, uint64 &out, bool &isIntConst)
{
    if (expr->GetOpCode() == OP_constval) {
        MIRConst *mirConst = static_cast<ConstvalNode *>(expr)->GetConstVal();
        if (mirConst->GetKind() == kConstInt) {
            out = static_cast<MIRIntConst *>(mirConst)->GetExtValue();
            isIntConst = true;
        }
        return nullptr;
    }
    BaseNode *foldExpr = nullptr;
    static ConstantFold cf(*theMIRModule);
    foldExpr = cf.Fold(expr);
    if (foldExpr != nullptr && foldExpr->GetOpCode() == OP_constval) {
        MIRConst *mirConst = static_cast<ConstvalNode *>(foldExpr)->GetConstVal();
        if (mirConst->GetKind() == kConstInt) {
            out = static_cast<MIRIntConst *>(mirConst)->GetExtValue();
            isIntConst = true;
        }
    }
    return foldExpr;
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

// Input a address expr, output a memEntry to abstract this expr
bool MemEntry::ComputeMemEntry(BaseNode &expr, MIRFunction &func, MemEntry &memEntry, bool isLowLevel)
{
    Opcode op = expr.GetOpCode();
    MIRType *memType = nullptr;
    switch (op) {
        case OP_dread: {
            const auto &concreteExpr = static_cast<const DreadNode &>(expr);
            auto *symbol = func.GetLocalOrGlobalSymbol(concreteExpr.GetStIdx());
            MIRType *curType = symbol->GetType();
            if (concreteExpr.GetFieldID() != 0) {
                curType = static_cast<MIRStructType *>(curType)->GetFieldType(concreteExpr.GetFieldID());
            }
            // Support kTypeScalar ptr if possible
            if (curType->GetKind() == kTypePointer) {
                memType = static_cast<MIRPtrType *>(curType)->GetPointedType();
            }
            break;
        }
        case OP_addrof: {
            const auto &concreteExpr = static_cast<const AddrofNode &>(expr);
            auto *symbol = func.GetLocalOrGlobalSymbol(concreteExpr.GetStIdx());
            MIRType *curType = symbol->GetType();
            if (concreteExpr.GetFieldID() != 0) {
                curType = static_cast<MIRStructType *>(curType)->GetFieldType(concreteExpr.GetFieldID());
            }
            memType = curType;
            break;
        }
        case OP_iread: {
            const auto &concreteExpr = static_cast<const IreadNode &>(expr);
            memType = concreteExpr.GetType();
            break;
        }
        case OP_iaddrof: {  // Do NOT call GetType because it is for OP_iread
            const auto &concreteExpr = static_cast<const IaddrofNode &>(expr);
            MIRType *curType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(concreteExpr.GetTyIdx());
            CHECK_FATAL(curType->IsMIRPtrType(), "must be MIRPtrType");
            curType = static_cast<MIRPtrType *>(curType)->GetPointedType();
            CHECK_FATAL(curType->IsStructType(), "must be MIRStructType");
            memType = static_cast<MIRStructType *>(curType)->GetFieldType(concreteExpr.GetFieldID());
            break;
        }
        case OP_regread: {
            if (isLowLevel && IsPrimitivePoint(expr.GetPrimType())) {
                memEntry.addrExpr = &expr;
                memEntry.memType =
                    nullptr;  // we cannot infer high level memory type, this is allowed for low level expand
                return true;
            }
            const auto &concreteExpr = static_cast<const RegreadNode &>(expr);
            MIRPreg *preg = func.GetPregItem(concreteExpr.GetRegIdx());
            bool isFromDread = (preg->GetOp() == OP_dread);
            bool isFromAddrof = (preg->GetOp() == OP_addrof);
            if (isFromDread || isFromAddrof) {
                auto *symbol = preg->rematInfo.sym;
                auto fieldId = preg->fieldID;
                MIRType *curType = symbol->GetType();
                if (fieldId != 0) {
                    curType = static_cast<MIRStructType *>(symbol->GetType())->GetFieldType(fieldId);
                }
                if (isFromDread && curType->GetKind() == kTypePointer) {
                    curType = static_cast<MIRPtrType *>(curType)->GetPointedType();
                }
                memType = curType;
            }
            break;
        }
        default: {
            if (isLowLevel && IsPrimitivePoint(expr.GetPrimType())) {
                memEntry.addrExpr = &expr;
                memEntry.memType =
                    nullptr;  // we cannot infer high level memory type, this is allowed for low level expand
                return true;
            }
            break;
        }
    }
    if (memType == nullptr) {
        return false;
    }
    memEntry.addrExpr = &expr;
    memEntry.memType = memType;
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

static void InsertBeforeAndMayPrintStmtList(BlockNode &block, const StmtNode &anchor, bool debug,
                                            std::initializer_list<StmtNode *> stmtList)
{
    for (StmtNode *stmt : stmtList) {
        if (stmt == nullptr) {
            continue;
        }
        block.InsertBefore(&anchor, stmt);
        if (debug) {
            stmt->Dump(0);
        }
    }
}

static bool NeedCheck(MemOpKind memOpKind)
{
    if (memOpKind == MEM_OP_memset_s || memOpKind == MEM_OP_memcpy_s) {
        return true;
    }
    return false;
}

// Create maple IR to check whether `expr` is a null pointer, IR is as follows:
//   brfalse @@n1 (ne u8 ptr (regread ptr %1, constval u64 0))
static CondGotoNode *CreateNullptrCheckStmt(BaseNode &expr, MIRFunction &func, MIRBuilder *mirBuilder,
                                            const MIRType &cmpResType, const MIRType &cmpOpndType)
{
    LabelIdx nullLabIdx = func.GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
    auto *checkExpr = mirBuilder->CreateExprCompare(OP_ne, cmpResType, cmpOpndType, &expr,
                                                    ConstructConstvalNode(0, PTY_u64, *mirBuilder));
    auto *checkStmt = mirBuilder->CreateStmtCondGoto(checkExpr, OP_brfalse, nullLabIdx);
    return checkStmt;
}

// Create maple IR to check whether `expr1` and `expr2` are equal
// brfalse @@a1 (ne u8 ptr (regread ptr %1, regread ptr %2))
static CondGotoNode *CreateAddressEqualCheckStmt(BaseNode &expr1, BaseNode &expr2, MIRFunction &func,
                                                 MIRBuilder *mirBuilder, const MIRType &cmpResType,
                                                 const MIRType &cmpOpndType)
{
    LabelIdx equalLabIdx = func.GetLabelTab()->CreateLabelWithPrefix('a');  // 'a' means address equal
    auto *checkExpr = mirBuilder->CreateExprCompare(OP_ne, cmpResType, cmpOpndType, &expr1, &expr2);
    auto *checkStmt = mirBuilder->CreateStmtCondGoto(checkExpr, OP_brfalse, equalLabIdx);
    return checkStmt;
}

// Create maple IR to check whether `expr1` and `expr2` are overlapped
// brfalse @@o1 (ge u8 ptr (
//   abs ptr (sub ptr (regread ptr %1, regread ptr %2)),
//   constval u64 xxx))
static CondGotoNode *CreateOverlapCheckStmt(BaseNode &expr1, BaseNode &expr2, uint32 size, MIRFunction &func,
                                            MIRBuilder *mirBuilder, const MIRType &cmpResType,
                                            const MIRType &cmpOpndType)
{
    LabelIdx overlapLabIdx = func.GetLabelTab()->CreateLabelWithPrefix('o');  // 'n' means overlap
    auto *checkExpr = mirBuilder->CreateExprCompare(
        OP_ge, cmpResType, cmpOpndType,
        mirBuilder->CreateExprUnary(OP_abs, cmpOpndType,
                                    mirBuilder->CreateExprBinary(OP_sub, cmpOpndType, &expr1, &expr2)),
        ConstructConstvalNode(size, PTY_u64, *mirBuilder));
    auto *checkStmt = mirBuilder->CreateStmtCondGoto(checkExpr, OP_brfalse, overlapLabIdx);
    return checkStmt;
}

// Generate IR to handle nullptr, IR is as follows:
//   @curLabel
//   regassign i32 %1 (constval i32 errNum)
//   goto @finalLabel
static void AddNullptrHandlerIR(const StmtNode &stmt, MIRBuilder *mirBuilder, BlockNode &block, StmtNode *retAssign,
                                LabelIdx curLabIdx, LabelIdx finalLabIdx, bool debug)
{
    auto *curLabelNode = mirBuilder->CreateStmtLabel(curLabIdx);
    auto *gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
    InsertBeforeAndMayPrintStmtList(block, stmt, debug, {curLabelNode, retAssign, gotoFinal});
}

static void AddMemsetCallStmt(const StmtNode &stmt, MIRFunction &func, BlockNode &block, BaseNode *addrExpr)
{
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    // call memset for dst memory when detecting overlapping
    MapleVector<BaseNode *> args(mirBuilder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.push_back(addrExpr);
    args.push_back(ConstructConstvalNode(0, PTY_i32, *mirBuilder));
    args.push_back(stmt.Opnd(1));
    auto *callMemset = mirBuilder->CreateStmtCall("memset", args);
    block.InsertBefore(&stmt, callMemset);
}

// Generate IR to handle errors that should be reset with memset, IR is as follows:
//   @curLabel
//   regassign i32 %1 (constval i32 errNum)
//   call memset  # new genrated memset will be expanded if possible
//   goto @finalLabel
static void AddResetHandlerIR(const StmtNode &stmt, MIRFunction &func, BlockNode &block, StmtNode *retAssign,
                              LabelIdx curLabIdx, LabelIdx finalLabIdx, BaseNode *addrExpr, bool debug)
{
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    auto *curLabelNode = mirBuilder->CreateStmtLabel(curLabIdx);
    InsertBeforeAndMayPrintStmtList(block, stmt, debug, {curLabelNode, retAssign});
    AddMemsetCallStmt(stmt, func, block, addrExpr);
    auto *gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
    InsertAndMayPrintStmt(block, stmt, debug, gotoFinal);
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

static void InsertCheckFailedBranch(MIRFunction &func, StmtNode &stmt, BlockNode &block, LabelIdx branchLabIdx,
                                    LabelIdx finalLabIdx, ErrorNumber errNumber, MemOpKind memOpKind, bool debug)
{
    auto mirBuilder = func.GetModule()->GetMIRBuilder();
    auto gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
    auto branchLabNode = mirBuilder->CreateStmtLabel(branchLabIdx);
    auto errnoAssign = MemEntry::GenMemopRetAssign(stmt, func, true, memOpKind, errNumber);
    InsertBeforeAndMayPrintStmtList(block, stmt, debug, {branchLabNode, errnoAssign, gotoFinal});
}

static StmtNode *InsertMemsetCallStmt(const MapleVector<BaseNode *> &args, MIRFunction &func, StmtNode &stmt,
                                      BlockNode &block, LabelIdx finalLabIdx, ErrorNumber errorNumber, bool debug)
{
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    auto *gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
    auto memsetFunc = mirBuilder->GetOrCreateFunction(kFuncNameOfMemset, TyIdx(PTY_void));
    auto memsetCallStmt = mirBuilder->CreateStmtCallAssigned(memsetFunc->GetPuidx(), args, nullptr, OP_callassigned);
    memsetCallStmt->SetSrcPos(stmt.GetSrcPos());
    auto *errnoAssign = MemEntry::GenMemopRetAssign(stmt, func, true, MEM_OP_memset_s, errorNumber);
    InsertBeforeAndMayPrintStmtList(block, stmt, debug, {memsetCallStmt, errnoAssign, gotoFinal});
    return memsetCallStmt;
}

static void CreateAndInsertCheckStmt(Opcode op, BaseNode *lhs, BaseNode *rhs, LabelIdx label, StmtNode &stmt,
                                     BlockNode &block, MIRFunction &func, bool debug)
{
    auto cmpResType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8));
    auto cmpU64Type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u64));
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    auto cmpStmt = mirBuilder->CreateExprCompare(op, *cmpResType, *cmpU64Type, lhs, rhs);
    auto checkStmt = mirBuilder->CreateStmtCondGoto(cmpStmt, OP_brtrue, label);
    checkStmt->SetBranchProb(kProbUnlikely);
    checkStmt->SetSrcPos(stmt.GetSrcPos());
    InsertAndMayPrintStmt(block, stmt, debug, checkStmt);
}

static StmtNode *ExpandOnSrcSizeGtDstSize(StmtNode &stmt, BlockNode &block, int64 srcSize, LabelIdx finalLabIdx,
                                          LabelIdx nullPtrLabIdx, MIRFunction &func, bool debug)
{
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    MapleVector<BaseNode *> args(func.GetCodeMempoolAllocator().Adapter());
    args.push_back(stmt.Opnd(kMemsetDstOpndIdx));
    args.push_back(stmt.Opnd(kMemsetSSrcOpndIdx));
    args.push_back(ConstructConstvalNode(srcSize, stmt.Opnd(kMemsetSSrcSizeOpndIdx)->GetPrimType(), *mirBuilder));
    auto memsetFunc = mirBuilder->GetOrCreateFunction(kFuncNameOfMemset, TyIdx(PTY_void));
    auto callStmt = mirBuilder->CreateStmtCallAssigned(memsetFunc->GetPuidx(), args, nullptr, OP_callassigned);
    callStmt->SetSrcPos(stmt.GetSrcPos());
    InsertAndMayPrintStmt(block, stmt, debug, callStmt);
    auto gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
    auto errnoAssign = MemEntry::GenMemopRetAssign(stmt, func, true, MEM_OP_memset_s, ERRNO_RANGE_AND_RESET);
    InsertBeforeAndMayPrintStmtList(block, stmt, debug, {errnoAssign, gotoFinal});
    InsertCheckFailedBranch(func, stmt, block, nullPtrLabIdx, finalLabIdx, ERRNO_INVAL, MEM_OP_memset_s, debug);
    auto *finalLabelNode = mirBuilder->CreateStmtLabel(finalLabIdx);
    InsertAndMayPrintStmt(block, stmt, debug, finalLabelNode);
    block.RemoveStmt(&stmt);
    return callStmt;
}

static void HandleZeroValueOfDstSize(StmtNode &stmt, BlockNode &block, int64 srcSize, int64 dstSize,
                                     LabelIdx finalLabIdx, LabelIdx dstSizeCheckLabIdx, MIRFunction &func,
                                     bool isDstSizeConst, bool debug)
{
    uint32 dstSizeOpndIdx = 1;  // only used by memset_s
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    if (!isDstSizeConst) {
        CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(dstSizeOpndIdx), ConstructConstvalNode(0, PTY_u64, *mirBuilder),
                                 dstSizeCheckLabIdx, stmt, block, func, debug);
    } else if (dstSize == 0) {
        auto gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
        auto errnoAssign = MemEntry::GenMemopRetAssign(stmt, func, true, MEM_OP_memset_s, ERRNO_RANGE);
        InsertBeforeAndMayPrintStmtList(block, stmt, debug, {errnoAssign, gotoFinal});
    }
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

// Lower memset(MemEntry, byte, size) into a series of assign stmts and replace callStmt in the block
// with these assign stmts
bool MemEntry::ExpandMemset(int64 byte, uint64 size, MIRFunction &func, StmtNode &stmt, BlockNode &block,
                            bool isLowLevel, bool debug, ErrorNumber errorNumber) const
{
    MemOpKind memOpKind = SimplifyMemOp::ComputeMemOpKind(stmt);
    MemEntryKind memKind = GetKind();
    // we don't check size equality in the low level expand
    if (!isLowLevel) {
        if (memKind == kMemEntryUnknown) {
            MayPrintLog(debug, false, memOpKind, "unsupported dst memory type, is it a bitfield?");
            return false;
        }
        if (memType->GetSize() != size) {
            MayPrintLog(debug, false, memOpKind, "dst size and size arg are not equal");
            return false;
        }
    }
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();

    if (isLowLevel) {  // For cglower, replace memset with a series of low-level iassignoff
        ExpandMemsetLowLevel(byte, size, func, stmt, block, memOpKind, debug, errorNumber);
        return true;
    }

    if (memKind == kMemEntryPrimitive) {
        BaseNode *rhsExpr = ConstructConstvalNode(byte, size, memType->GetPrimType(), *mirBuilder);
        StmtNode *newAssign = nullptr;
        if (addrExpr->GetOpCode() == OP_addrof) {  // We prefer dassign to iassign
            auto *addrof = static_cast<AddrofNode *>(addrExpr);
            auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
            newAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID(), rhsExpr);
        } else {
            MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
            newAssign = mirBuilder->CreateStmtIassign(*memPtrType, 0, addrExpr, rhsExpr);
        }
        InsertAndMayPrintStmt(block, stmt, debug, newAssign);
    } else if (memKind == kMemEntryStruct) {
        auto *structType = static_cast<MIRStructType *>(memType);
        // struct size should be small enough, struct field size should be big enough
        constexpr uint32 maxStructSize = 64;  // in byte
        constexpr uint32 minFieldSize = 4;    // in byte
        size_t structSize = structType->GetSize();
        size_t numFields = structType->NumberOfFieldIDs();
        // Relax restrictions when store-merge is powerful enough
        bool expandIt =
            (structSize <= maxStructSize && (structSize / numFields >= minFieldSize) && !structType->HasPadding());
        if (!expandIt) {
            // We only expand memset for no-padding struct, because only in this case, element-wise and byte-wise
            // are equivalent
            MayPrintLog(debug, false, memOpKind,
                        "struct type has padding, or struct sum size is too big, or filed size is too small");
            return false;
        }
        bool hasArrayField = false;
        for (uint32 id = 1; id <= numFields; ++id) {
            auto *fieldType = structType->GetFieldType(id);
            if (fieldType->GetKind() == kTypeArray) {
                hasArrayField = true;
                break;
            }
        }
        if (hasArrayField) {
            // struct with array fields is not supported to expand for now, enhance it when needed
            MayPrintLog(debug, false, memOpKind, "struct with array fields is not supported to expand");
            return false;
        }

        // Build assign for each fields in the struct type
        // We should skip union fields
        for (FieldID id = 1; static_cast<size_t>(id) <= numFields; ++id) {
            MIRType *fieldType = structType->GetFieldType(id);
            // We only consider leaf field with valid type size
            if (fieldType->GetSize() == 0 || fieldType->GetPrimType() == PTY_agg) {
                continue;
            }
            if (fieldType->GetKind() == kTypeBitField &&
                static_cast<MIRBitFieldType *>(fieldType)->GetFieldSize() == 0) {
                continue;
            }
            // now the fieldType is primitive type
            BaseNode *rhsExpr =
                ConstructConstvalNode(byte, fieldType->GetSize(), fieldType->GetPrimType(), *mirBuilder);
            StmtNode *fieldAssign = nullptr;
            if (addrExpr->GetOpCode() == OP_addrof) {
                auto *addrof = static_cast<AddrofNode *>(addrExpr);
                auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
                fieldAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID() + id, rhsExpr);
            } else {
                MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
                fieldAssign = mirBuilder->CreateStmtIassign(*memPtrType, id, addrExpr, rhsExpr);
            }
            InsertAndMayPrintStmt(block, stmt, debug, fieldAssign);
        }
    } else if (memKind == kMemEntryArray) {
        // We only consider array with dim == 1 now, and element type must be primitive type
        auto *arrayType = static_cast<MIRArrayType *>(memType);
        if (arrayType->GetDim() != 1 || (arrayType->GetElemType()->GetKind() != kTypeScalar &&
                                         arrayType->GetElemType()->GetKind() != kTypePointer)) {
            MayPrintLog(debug, false, memOpKind, "array dim != 1 or array elements are not primtive type");
            return false;
        }
        MIRType *elemType = arrayType->GetElemType();
        if (elemType->GetSize() < k4BitSize) {
            MayPrintLog(debug, false, memOpKind,
                        "element size < 4, don't expand it to  avoid to genearte lots of strb/strh");
            return false;
        }
        uint64 elemCnt = static_cast<uint64>(arrayType->GetSizeArrayItem(0));
        if (elemType->GetSize() * elemCnt != size) {
            MayPrintLog(debug, false, memOpKind, "array size not equal");
            return false;
        }
        for (size_t i = 0; i < elemCnt; ++i) {
            BaseNode *indexExpr = ConstructConstvalNode(i, PTY_u32, *mirBuilder);
            auto *arrayExpr = mirBuilder->CreateExprArray(*arrayType, addrExpr, indexExpr);
            auto *newValOpnd = ConstructConstvalNode(byte, elemType->GetSize(), elemType->GetPrimType(), *mirBuilder);
            MIRType *elemPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType);
            auto *arrayElementAssign = mirBuilder->CreateStmtIassign(*elemPtrType, 0, arrayExpr, newValOpnd);
            InsertAndMayPrintStmt(block, stmt, debug, arrayElementAssign);
        }
    } else {
        CHECK_FATAL(false, "impossible");
    }

    // handle memset return val
    auto *retAssign = GenMemopRetAssign(stmt, func, isLowLevel, memOpKind, errorNumber);
    InsertAndMayPrintStmt(block, stmt, debug, retAssign);
    block.RemoveStmt(&stmt);
    return true;
}

static std::pair<StmtNode *, StmtNode *> GenerateMemoryCopyPair(MIRBuilder *mirBuilder, BaseNode *rhs, BaseNode *lhs,
                                                                uint32 offset, uint32 curSize, PregIdx tmpRegIdx)
{
    auto *ptrType = GlobalTables::GetTypeTable().GetPtrType();
    PrimType constType = GetIntegerPrimTypeBySizeAndSign(curSize * 8, false);
    MIRType *constMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(constType));
    auto *constMIRPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*constMIRType);
    BaseNode *rhsAddrExpr = rhs;
    if (offset != 0) {
        auto *offsetConstExpr = ConstructConstvalNode(offset, PTY_u64, *mirBuilder);
        rhsAddrExpr = mirBuilder->CreateExprBinary(OP_add, *ptrType, rhs, offsetConstExpr);
    }
    BaseNode *rhsExpr = mirBuilder->CreateExprIread(*constMIRType, *constMIRPtrType, 0, rhsAddrExpr);
    auto *regassign = mirBuilder->CreateStmtRegassign(PTY_u64, tmpRegIdx, rhsExpr);
    auto *iassignoff = mirBuilder->CreateStmtIassignoff(constType, static_cast<int32>(offset), lhs,
                                                        mirBuilder->CreateExprRegread(PTY_u64, tmpRegIdx));
    return {regassign, iassignoff};
}

void MemEntry::ExpandMemcpyLowLevel(const MemEntry &srcMem, uint64 copySize, MIRFunction &func, StmtNode &stmt,
                                    BlockNode &block, MemOpKind memOpKind, bool debug, ErrorNumber errorNumber) const
{
    if (errorNumber == ERRNO_RANGE) {
        auto *retAssign = GenMemopRetAssign(stmt, func, true, memOpKind, errorNumber);
        InsertAndMayPrintStmt(block, stmt, debug, retAssign);
        block.RemoveStmt(&stmt);
        return;
    }
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    std::vector<uint32> blocks;
    SplitMemoryIntoBlocks(copySize, blocks);
    uint32 offset = 0;
    // If blocks.size() > 1 and `src` or `dst` is not a leaf node,
    // we should extract common expr to avoid redundant expression
    BaseNode *realSrcExpr = srcMem.addrExpr;
    BaseNode *realDstExpr = addrExpr;
    if (blocks.size() > 1) {
        realDstExpr = TryToExtractComplexExpr(addrExpr, func, block, stmt, debug);
        realSrcExpr = TryToExtractComplexExpr(srcMem.addrExpr, func, block, stmt, debug);
    }
    auto *ptrType = GlobalTables::GetTypeTable().GetPtrType();
    LabelIdx dstNullLabIdx;
    LabelIdx srcNullLabIdx;
    LabelIdx overlapLabIdx;
    LabelIdx addressEqualLabIdx;
    if (NeedCheck(memOpKind)) {
        auto *cmpResType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_u8));
        auto *cmpOpndType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ptr));
        // check dst != NULL
        auto *checkDstStmt = CreateNullptrCheckStmt(*realDstExpr, func, mirBuilder, *cmpResType, *cmpOpndType);
        dstNullLabIdx = checkDstStmt->GetOffset();
        // check src != NULL
        auto *checkSrcStmt = CreateNullptrCheckStmt(*realSrcExpr, func, mirBuilder, *cmpResType, *cmpOpndType);
        srcNullLabIdx = checkSrcStmt->GetOffset();
        InsertBeforeAndMayPrintStmtList(block, stmt, debug, {checkDstStmt, checkSrcStmt});
        if (errorNumber != ERRNO_RANGE_AND_RESET) {
            // check src == dst
            auto *checkAddrEqualStmt =
                CreateAddressEqualCheckStmt(*realDstExpr, *realSrcExpr, func, mirBuilder, *cmpResType, *cmpOpndType);
            addressEqualLabIdx = checkAddrEqualStmt->GetOffset();
            // check overlap
            auto *checkOverlapStmt = CreateOverlapCheckStmt(*realDstExpr, *realSrcExpr, static_cast<uint32>(copySize),
                                                            func, mirBuilder, *cmpResType, *cmpOpndType);
            overlapLabIdx = checkOverlapStmt->GetOffset();
            InsertBeforeAndMayPrintStmtList(block, stmt, debug, {checkAddrEqualStmt, checkOverlapStmt});
        }
    }
    if (errorNumber == ERRNO_RANGE_AND_RESET) {
        AddMemsetCallStmt(stmt, func, block, addrExpr);
    } else {
        // memory copy optimization
        PregIdx tmpRegIdx1 = 0;
        PregIdx tmpRegIdx2 = 0;
        for (uint32 i = 0; i < blocks.size(); ++i) {
            uint32 curSize = blocks[i];
            bool canMergedWithNextSize = (i + 1 < blocks.size()) && blocks[i + 1] == curSize;
            if (!canMergedWithNextSize) {
                // low level memcpy expand result:
                // It seems ireadoff has not been supported by cg HandleFunction, so we use iread instead of ireadoff
                // [not support] iassignoff <prim-type> <offset> (dstAddrExpr, ireadoff <prim-type> <offset>
                // (srcAddrExpr)) [ok] iassignoff <prim-type> <offset> (dstAddrExpr, iread <prim-type> <type> (add ptr
                // (srcAddrExpr, offset)))
                PrimType constType = GetIntegerPrimTypeBySizeAndSign(curSize * 8, false);
                MIRType *constMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(constType));
                auto *constMIRPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*constMIRType);
                BaseNode *rhsAddrExpr = realSrcExpr;
                if (offset != 0) {
                    auto *offsetConstExpr = ConstructConstvalNode(offset, PTY_u64, *mirBuilder);
                    rhsAddrExpr = mirBuilder->CreateExprBinary(OP_add, *ptrType, realSrcExpr, offsetConstExpr);
                }
                BaseNode *rhsExpr = mirBuilder->CreateExprIread(*constMIRType, *constMIRPtrType, 0, rhsAddrExpr);
                auto *iassignoff = mirBuilder->CreateStmtIassignoff(constType, offset, realDstExpr, rhsExpr);
                InsertAndMayPrintStmt(block, stmt, debug, iassignoff);
                offset += curSize;
                continue;
            }

            // merge two str/ldr into a stp/ldp
            if (tmpRegIdx1 == 0 || tmpRegIdx2 == 0) {
                tmpRegIdx1 = func.GetPregTab()->CreatePreg(PTY_u64);
                tmpRegIdx2 = func.GetPregTab()->CreatePreg(PTY_u64);
            }
            auto pair1 = GenerateMemoryCopyPair(mirBuilder, realSrcExpr, realDstExpr, offset, curSize, tmpRegIdx1);
            auto pair2 =
                GenerateMemoryCopyPair(mirBuilder, realSrcExpr, realDstExpr, offset + curSize, curSize, tmpRegIdx2);
            // insert order: regassign1, regassign2, iassignoff1, iassignoff2
            InsertBeforeAndMayPrintStmtList(block, stmt, debug, {pair1.first, pair2.first, pair1.second, pair2.second});
            offset += (curSize << 1);
            ++i;
        }
    }
    // handle memcpy return val
    auto *retAssign = GenMemopRetAssign(stmt, func, true, memOpKind, errorNumber);
    InsertAndMayPrintStmt(block, stmt, debug, retAssign);
    if (NeedCheck(memOpKind)) {
        LabelIdx finalLabIdx = func.GetLabelTab()->CreateLabelWithPrefix('f');
        auto *finalLabelNode = mirBuilder->CreateStmtLabel(finalLabIdx);
        // Add goto final stmt for expanded body
        auto *gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
        InsertAndMayPrintStmt(block, stmt, debug, gotoFinal);
        // Add handler IR if dst == NULL
        auto *dstErrAssign = GenMemopRetAssign(stmt, func, true, memOpKind, ERRNO_INVAL);
        AddNullptrHandlerIR(stmt, mirBuilder, block, dstErrAssign, dstNullLabIdx, finalLabIdx, debug);
        // Add handler IR if src == NULL
        auto *srcErrAssign = GenMemopRetAssign(stmt, func, true, memOpKind, ERRNO_INVAL_AND_RESET);
        AddResetHandlerIR(stmt, func, block, srcErrAssign, srcNullLabIdx, finalLabIdx, addrExpr, debug);
        if (errorNumber != ERRNO_RANGE_AND_RESET) {
            // Add handler IR if dst == src
            auto *addrEqualAssign = GenMemopRetAssign(stmt, func, true, memOpKind, ERRNO_OK);
            AddNullptrHandlerIR(stmt, mirBuilder, block, addrEqualAssign, addressEqualLabIdx, finalLabIdx, debug);
            // Add handler IR if dst and src are overlapped
            auto *overlapErrAssign = GenMemopRetAssign(stmt, func, true, memOpKind, ERRNO_OVERLAP_AND_RESET);
            AddResetHandlerIR(stmt, func, block, overlapErrAssign, overlapLabIdx, finalLabIdx, addrExpr, debug);
        }
        InsertAndMayPrintStmt(block, stmt, debug, finalLabelNode);
    }
    block.RemoveStmt(&stmt);
}

bool MemEntry::ExpandMemcpy(const MemEntry &srcMem, uint64 copySize, MIRFunction &func, StmtNode &stmt,
                            BlockNode &block, bool isLowLevel, bool debug, ErrorNumber errorNumber) const
{
    MemOpKind memOpKind = SimplifyMemOp::ComputeMemOpKind(stmt);
    MemEntryKind memKind = GetKind();
    if (!isLowLevel) {  // check type consistency and memKind only for high level expand
        if (memOpKind == MEM_OP_memcpy_s) {
            MayPrintLog(debug, false, memOpKind, "all memcpy_s will be handed by cglower");
            return false;
        }
        if (memType != srcMem.memType) {
            return false;
        }
        CHECK_FATAL(memKind != kMemEntryUnknown, "invalid memKind");
    }
    MIRBuilder *mirBuilder = func.GetModule()->GetMIRBuilder();
    StmtNode *newAssign = nullptr;
    if (isLowLevel) {  // For cglower, replace memcpy with a series of low-level iassignoff
        ExpandMemcpyLowLevel(srcMem, copySize, func, stmt, block, memOpKind, debug, errorNumber);
        return true;
    }

    if (memKind == kMemEntryPrimitive || memKind == kMemEntryStruct) {
        // Do low level expand for all struct memcpy for now
        if (memKind == kMemEntryStruct) {
            MayPrintLog(debug, false, memOpKind, "Do low level expand for all struct memcpy for now");
            return false;
        }
        if (addrExpr->GetOpCode() == OP_addrof) {  // We prefer dassign to iassign
            auto *addrof = static_cast<AddrofNode *>(addrExpr);
            auto *symbol = func.GetLocalOrGlobalSymbol(addrof->GetStIdx());
            newAssign = mirBuilder->CreateStmtDassign(*symbol, addrof->GetFieldID(), srcMem.BuildAsRhsExpr(func));
        } else {
            MIRType *memPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*memType);
            newAssign = mirBuilder->CreateStmtIassign(*memPtrType, 0, addrExpr, srcMem.BuildAsRhsExpr(func));
        }
        InsertAndMayPrintStmt(block, stmt, debug, newAssign);
        // split struct agg copy
        if (memKind == kMemEntryStruct) {
            if (newAssign->GetOpCode() == OP_dassign) {
                (void)SplitDassignAggCopy(static_cast<DassignNode *>(newAssign), &block, &func);
            } else if (newAssign->GetOpCode() == OP_iassign) {
                (void)SplitIassignAggCopy(static_cast<IassignNode *>(newAssign), &block, &func);
            } else {
                CHECK_FATAL(false, "impossible");
            }
        }
    } else if (memKind == kMemEntryArray) {
        // We only consider array with dim == 1 now, and element type must be primitive type
        auto *arrayType = static_cast<MIRArrayType *>(memType);
        if (arrayType->GetDim() != 1 || (arrayType->GetElemType()->GetKind() != kTypeScalar &&
                                         arrayType->GetElemType()->GetKind() != kTypePointer)) {
            MayPrintLog(debug, false, memOpKind, "array dim != 1 or array elements are not primtive type");
            return false;
        }
        MIRType *elemType = arrayType->GetElemType();
        if (elemType->GetSize() < k4ByteSize) {
            MayPrintLog(debug, false, memOpKind,
                        "element size < 4, don't expand it to avoid to genearte lots of strb/strh");
            return false;
        }
        size_t elemCnt = arrayType->GetSizeArrayItem(0);
        if (elemType->GetSize() * elemCnt != copySize) {
            MayPrintLog(debug, false, memOpKind, "array size not equal");
            return false;
        }
        // if srcExpr is too complex (for example: addrof expr of global/static array), let cg expand it
        if (elemCnt > 1 && IsComplexExpr(srcMem.addrExpr, func)) {
            MayPrintLog(debug, false, memOpKind, "srcExpr is too complex, let cg expand it to avoid redundant inst");
            return false;
        }
        MIRType *elemPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType);
        MIRType *u32Type = GlobalTables::GetTypeTable().GetUInt32();
        for (size_t i = 0; i < elemCnt; ++i) {
            ConstvalNode *indexExpr = mirBuilder->CreateConstval(
                GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(i), *u32Type));
            auto *arrayExpr = mirBuilder->CreateExprArray(*arrayType, addrExpr, indexExpr);
            auto *rhsArrayExpr = mirBuilder->CreateExprArray(*arrayType, srcMem.addrExpr, indexExpr);
            auto *rhsIreadExpr = mirBuilder->CreateExprIread(*elemType, *elemPtrType, 0, rhsArrayExpr);
            auto *arrayElemAssign = mirBuilder->CreateStmtIassign(*elemPtrType, 0, arrayExpr, rhsIreadExpr);
            InsertAndMayPrintStmt(block, stmt, debug, arrayElemAssign);
        }
    } else {
        CHECK_FATAL(false, "impossible");
    }

    // handle memcpy return val
    auto *retAssign = GenMemopRetAssign(stmt, func, isLowLevel, memOpKind, errorNumber);
    InsertAndMayPrintStmt(block, stmt, debug, retAssign);
    block.RemoveStmt(&stmt);
    return true;
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
    MemOpKind memOpKind = ComputeMemOpKind(stmt);
    switch (memOpKind) {
        case MEM_OP_memset:
        case MEM_OP_memset_s: {
            return SimplifyMemset(stmt, block, isLowLevel);
        }
        case MEM_OP_memcpy:
        case MEM_OP_memcpy_s: {
            return SimplifyMemcpy(stmt, block, isLowLevel);
        }
        default:
            break;
    }
    return false;
}

// expand memset_s call statement, return pointer of memset call statement node to be expanded in the next step, return
// nullptr if memset_s is expanded completely.
StmtNode *SimplifyMemOp::PartiallyExpandMemsetS(StmtNode &stmt, BlockNode &block)
{
    ErrorNumber errNum = ERRNO_OK;

    uint64 srcSize = 0;
    bool isSrcSizeConst = false;
    BaseNode *foldSrcSizeExpr = FoldIntConst(stmt.Opnd(kMemsetSSrcSizeOpndIdx), srcSize, isSrcSizeConst);
    if (foldSrcSizeExpr != nullptr) {
        stmt.SetOpnd(foldSrcSizeExpr, kMemsetSDstSizeOpndIdx);
    }

    uint64 dstSize = 0;
    bool isDstSizeConst = false;
    BaseNode *foldDstSizeExpr = FoldIntConst(stmt.Opnd(kMemsetSDstSizeOpndIdx), dstSize, isDstSizeConst);
    if (foldDstSizeExpr != nullptr) {
        stmt.SetOpnd(foldDstSizeExpr, kMemsetSDstSizeOpndIdx);
    }
    if (isDstSizeConst) {
        if ((srcSize > dstSize && dstSize == 0) || dstSize > kSecurecMemMaxLen) {
            errNum = ERRNO_RANGE;
        }
    }

    MIRBuilder *mirBuilder = func->GetModule()->GetMIRBuilder();
    LabelIdx finalLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('f');
    if (errNum != ERRNO_OK) {
        auto errnoAssign = MemEntry::GenMemopRetAssign(stmt, *func, true, MEM_OP_memset_s, errNum);
        InsertAndMayPrintStmt(block, stmt, debug, errnoAssign);
        block.RemoveStmt(&stmt);
        return nullptr;
    } else {
        LabelIdx dstSizeCheckLabIdx, srcSizeCheckLabIdx, nullPtrLabIdx;
        if (!isDstSizeConst) {
            // check if dst size is greater than maxlen
            dstSizeCheckLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
            CreateAndInsertCheckStmt(OP_gt, stmt.Opnd(kMemsetSDstSizeOpndIdx),
                                     ConstructConstvalNode(kSecurecMemMaxLen, PTY_u64, *mirBuilder), dstSizeCheckLabIdx,
                                     stmt, block, *func, debug);
        }

        // check if dst is nullptr
        nullPtrLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
        CreateAndInsertCheckStmt(OP_eq, stmt.Opnd(kMemsetDstOpndIdx), ConstructConstvalNode(0, PTY_u64, *mirBuilder),
                                 nullPtrLabIdx, stmt, block, *func, debug);

        if (isDstSizeConst && isSrcSizeConst) {
            if (srcSize > dstSize) {
                srcSize = dstSize;
                return ExpandOnSrcSizeGtDstSize(stmt, block, srcSize, finalLabIdx, nullPtrLabIdx, *func, debug);
            }
        } else {
            // check if src size is greater than dst size
            srcSizeCheckLabIdx = func->GetLabelTab()->CreateLabelWithPrefix('n');  // 'n' means nullptr
            CreateAndInsertCheckStmt(OP_gt, stmt.Opnd(kMemsetSSrcSizeOpndIdx), stmt.Opnd(kMemsetSDstSizeOpndIdx),
                                     srcSizeCheckLabIdx, stmt, block, *func, debug);
        }

        MapleVector<BaseNode *> args(func->GetCodeMempoolAllocator().Adapter());
        args.push_back(stmt.Opnd(kMemsetDstOpndIdx));
        args.push_back(stmt.Opnd(kMemsetSSrcOpndIdx));
        args.push_back(stmt.Opnd(kMemsetSSrcSizeOpndIdx));
        auto memsetCallStmt = InsertMemsetCallStmt(args, *func, stmt, block, finalLabIdx, errNum, debug);

        if (!isSrcSizeConst || !isDstSizeConst) {
            // handle src size error
            auto branchLabNode = mirBuilder->CreateStmtLabel(srcSizeCheckLabIdx);
            InsertAndMayPrintStmt(block, stmt, debug, branchLabNode);
            HandleZeroValueOfDstSize(stmt, block, srcSize, dstSize, finalLabIdx, dstSizeCheckLabIdx, *func,
                                     isDstSizeConst, debug);
            args.pop_back();
            args.push_back(stmt.Opnd(kMemsetSDstSizeOpndIdx));
            (void)InsertMemsetCallStmt(args, *func, stmt, block, finalLabIdx, ERRNO_RANGE_AND_RESET, debug);
        }

        // handle dst nullptr error
        auto nullptrLabNode = mirBuilder->CreateStmtLabel(nullPtrLabIdx);
        InsertAndMayPrintStmt(block, stmt, debug, nullptrLabNode);
        HandleZeroValueOfDstSize(stmt, block, srcSize, dstSize, finalLabIdx, dstSizeCheckLabIdx, *func, isDstSizeConst,
                                 debug);
        auto gotoFinal = mirBuilder->CreateStmtGoto(OP_goto, finalLabIdx);
        auto errnoAssign = MemEntry::GenMemopRetAssign(stmt, *func, true, MEM_OP_memset_s, ERRNO_INVAL);
        InsertBeforeAndMayPrintStmtList(block, stmt, debug, {errnoAssign, gotoFinal});

        if (!isDstSizeConst) {
            // handle dst size error
            InsertCheckFailedBranch(*func, stmt, block, dstSizeCheckLabIdx, finalLabIdx, ERRNO_RANGE, MEM_OP_memset_s,
                                    debug);
        }
        auto *finalLabelNode = mirBuilder->CreateStmtLabel(finalLabIdx);
        InsertAndMayPrintStmt(block, stmt, debug, finalLabelNode);
        block.RemoveStmt(&stmt);
        return memsetCallStmt;
    }
}

// Try to replace the call to memset with a series of assign operations (including dassign, iassign, iassignoff), which
// is usually profitable for small memory size.
// This function is called in two places, one in mpl2mpl simplify, another in cglower:
// (1) mpl2mpl memset expand (isLowLevel == false)
//   for primitive type, array type with element size < 4 bytes and struct type without padding
// (2) cglower memset expand
//   for array type with element size >= 4 bytes and struct type with paddings
bool SimplifyMemOp::SimplifyMemset(StmtNode &stmt, BlockNode &block, bool isLowLevel)
{
    MemOpKind memOpKind = ComputeMemOpKind(stmt);
    if (memOpKind != MEM_OP_memset && memOpKind != MEM_OP_memset_s) {
        return false;
    }
    uint32 dstOpndIdx = 0;
    uint32 srcOpndIdx = 1;
    uint32 srcSizeOpndIdx = 2;
    bool isSafeVersion = memOpKind == MEM_OP_memset_s;
    if (debug) {
        LogInfo::MapleLogger() << "[funcName] " << func->GetName() << std::endl;
        stmt.Dump(0);
    }

    StmtNode *memsetCallStmt = &stmt;
    if (memOpKind == MEM_OP_memset_s && !isLowLevel) {
        memsetCallStmt = PartiallyExpandMemsetS(stmt, block);
        if (!memsetCallStmt) {
            return true;  // Expand memset_s completely, no extra memset is generated, so just return true
        }
    }

    uint64 srcSize = 0;
    bool isSrcSizeConst = false;
    BaseNode *foldSrcSizeExpr = FoldIntConst(memsetCallStmt->Opnd(srcSizeOpndIdx), srcSize, isSrcSizeConst);
    if (foldSrcSizeExpr != nullptr) {
        memsetCallStmt->SetOpnd(foldSrcSizeExpr, srcSizeOpndIdx);
    }

    if (isSrcSizeConst) {
        // If the size is too big, we won't expand it
        uint32 thresholdExpand = (isSafeVersion ? thresholdMemsetSExpand : thresholdMemsetExpand);
        if (srcSize > thresholdExpand) {
            MayPrintLog(debug, false, memOpKind, "size is too big");
            return false;
        }
        if (srcSize == 0) {
            if (memOpKind == MEM_OP_memset) {
                auto *retAssign = MemEntry::GenMemopRetAssign(*memsetCallStmt, *func, isLowLevel, memOpKind);
                InsertAndMayPrintStmt(block, *memsetCallStmt, debug, retAssign);
            }
            block.RemoveStmt(memsetCallStmt);
            return true;
        }
    }

    // memset's 'src size' must be a const value, otherwise we can not expand it
    if (!isSrcSizeConst) {
        MayPrintLog(debug, false, memOpKind, "size is not int const");
        return false;
    }

    ErrorNumber errNum = ERRNO_OK;

    uint64 val = 0;
    bool isIntConst = false;
    BaseNode *foldValExpr = FoldIntConst(memsetCallStmt->Opnd(srcOpndIdx), val, isIntConst);
    if (foldValExpr != nullptr) {
        memsetCallStmt->SetOpnd(foldValExpr, srcOpndIdx);
    }
    // memset's second argument 'val' should also be a const value
    if (!isIntConst) {
        MayPrintLog(debug, false, memOpKind, "val is not int const");
        return false;
    }

    MemEntry dstMemEntry;
    bool valid = MemEntry::ComputeMemEntry(*(memsetCallStmt->Opnd(dstOpndIdx)), *func, dstMemEntry, isLowLevel);
    if (!valid) {
        MayPrintLog(debug, false, memOpKind, "dstMemEntry is invalid");
        return false;
    }
    bool ret = false;
    if (srcSize != 0) {
        ret = dstMemEntry.ExpandMemset(val, static_cast<uint64>(srcSize), *func, *memsetCallStmt, block, isLowLevel,
                                       debug, errNum);
    } else {
        // if size == 0, no need to set memory, just return error nummber
        auto *retAssign = MemEntry::GenMemopRetAssign(*memsetCallStmt, *func, isLowLevel, memOpKind, errNum);
        InsertAndMayPrintStmt(block, *memsetCallStmt, debug, retAssign);
        block.RemoveStmt(memsetCallStmt);
        ret = true;
    }
    if (ret) {
        MayPrintLog(debug, true, memOpKind, "well done");
    }
    return ret;
}

bool SimplifyMemOp::SimplifyMemcpy(StmtNode &stmt, BlockNode &block, bool isLowLevel)
{
    MemOpKind memOpKind = ComputeMemOpKind(stmt);
    if (memOpKind != MEM_OP_memcpy && memOpKind != MEM_OP_memcpy_s) {
        return false;
    }
    uint32 dstOpndIdx = 0;
    uint32 dstSizeOpndIdx = kFirstOpnd;  // only used by memcpy_s
    uint32 srcOpndIdx = kSecondOpnd;
    uint32 srcSizeOpndIdx = kThirdOpnd;
    bool isSafeVersion = memOpKind == MEM_OP_memcpy_s;
    if (isSafeVersion) {
        dstSizeOpndIdx = kSecondOpnd;
        srcOpndIdx = kThirdOpnd;
        srcSizeOpndIdx = kFourthOpnd;
    }
    if (debug) {
        LogInfo::MapleLogger() << "[funcName] " << func->GetName() << std::endl;
        stmt.Dump(0);
    }

    uint64 srcSize = 0;
    bool isIntConst = false;
    BaseNode *foldCopySizeExpr = FoldIntConst(stmt.Opnd(srcSizeOpndIdx), srcSize, isIntConst);
    if (foldCopySizeExpr != nullptr) {
        stmt.SetOpnd(foldCopySizeExpr, srcSizeOpndIdx);
    }
    if (!isIntConst) {
        MayPrintLog(debug, false, memOpKind, "src size is not an int const");
        return false;
    }
    uint32 thresholdExpand = (isSafeVersion ? thresholdMemcpySExpand : thresholdMemcpyExpand);
    if (srcSize > thresholdExpand) {
        MayPrintLog(debug, false, memOpKind, "size is too big");
        return false;
    }
    if (srcSize == 0) {
        MayPrintLog(debug, false, memOpKind, "memcpy with src size 0");
        return false;
    }
    uint64 copySize = srcSize;
    ErrorNumber errNum = ERRNO_OK;
    if (isSafeVersion) {
        uint64 dstSize = 0;
        bool isDstSizeConst = false;
        BaseNode *foldDstSizeExpr = FoldIntConst(stmt.Opnd(dstSizeOpndIdx), dstSize, isDstSizeConst);
        if (foldDstSizeExpr != nullptr) {
            stmt.SetOpnd(foldDstSizeExpr, dstSizeOpndIdx);
        }
        if (!isDstSizeConst) {
            MayPrintLog(debug, false, memOpKind, "dst size is not int const");
            return false;
        }
        if (dstSize == 0 || dstSize > kSecurecMemMaxLen) {
            copySize = 0;
            errNum = ERRNO_RANGE;
        } else if (srcSize > dstSize) {
            copySize = dstSize;
            errNum = ERRNO_RANGE_AND_RESET;
        }
    }

    MemEntry dstMemEntry;
    bool valid = MemEntry::ComputeMemEntry(*stmt.Opnd(dstOpndIdx), *func, dstMemEntry, isLowLevel);
    if (!valid) {
        MayPrintLog(debug, false, memOpKind, "dstMemEntry is invalid");
        return false;
    }
    MemEntry srcMemEntry;
    valid = MemEntry::ComputeMemEntry(*stmt.Opnd(srcOpndIdx), *func, srcMemEntry, isLowLevel);
    if (!valid) {
        MayPrintLog(debug, false, memOpKind, "srcMemEntry is invalid");
        return false;
    }
    // We don't check type consistency when doing low level expand
    if (!isLowLevel) {
        if (dstMemEntry.memType != srcMemEntry.memType) {
            MayPrintLog(debug, false, memOpKind, "dst and src have different type");
            return false;  // entryType must be identical
        }
        if (dstMemEntry.memType->GetSize() != static_cast<uint64>(srcSize)) {
            MayPrintLog(debug, false, memOpKind, "copy size != dst memory size");
            return false;  // copy size should equal to dst memory size, we maybe allow smaller copy size later
        }
    }
    bool ret = false;
    if (copySize != 0) {
        ret = dstMemEntry.ExpandMemcpy(srcMemEntry, copySize, *func, stmt, block, isLowLevel, debug, errNum);
    } else {
        // if copySize == 0, no need to copy memory, just return error number
        auto *retAssign = MemEntry::GenMemopRetAssign(stmt, *func, isLowLevel, memOpKind, errNum);
        InsertAndMayPrintStmt(block, stmt, debug, retAssign);
        block.RemoveStmt(&stmt);
        ret = true;
    }
    if (ret) {
        MayPrintLog(debug, true, memOpKind, "well done");
    }
    return ret;
}

}  // namespace maple
