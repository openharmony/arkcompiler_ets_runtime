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

#include "ext_constantfold.h"
#include <climits>

namespace maple {
// This class is designed to further identify simplification
// patterns that have not been covered in ConstantFold.

StmtNode *ExtConstantFold::ExtSimplify(StmtNode *node)
{
    CHECK_NULL_FATAL(node);
    switch (node->GetOpCode()) {
        case OP_block:
            return ExtSimplifyBlock(static_cast<BlockNode *>(node));
        case OP_if:
            return ExtSimplifyIf(static_cast<IfStmtNode *>(node));
        case OP_dassign:
            return ExtSimplifyDassign(static_cast<DassignNode *>(node));
        case OP_iassign:
            return ExtSimplifyIassign(static_cast<IassignNode *>(node));
        case OP_dowhile:
        case OP_while:
            return ExtSimplifyWhile(static_cast<WhileStmtNode *>(node));
        default:
            return node;
    }
}

BaseNode *ExtConstantFold::DispatchFold(BaseNode *node)
{
    // Not trying all possiblities.
    // For simplicity, stop looking further down the expression once OP_OP_cior/OP_cand (etc) are seen
    CHECK_NULL_FATAL(node);
    switch (node->GetOpCode()) {
        case OP_cior:
        case OP_lior:
            return ExtFoldIor(static_cast<BinaryNode *>(node));
        case OP_cand:
        case OP_land:
            return ExtFoldXand(static_cast<BinaryNode *>(node));
        case OP_abs:
        case OP_bnot:
        case OP_lnot:
        case OP_neg:
        case OP_recip:
        case OP_sqrt:
            return ExtFoldUnary(static_cast<UnaryNode *>(node));
        case OP_add:
        case OP_ashr:
        case OP_band:
        case OP_bior:
        case OP_bxor:
        case OP_div:
        case OP_lshr:
        case OP_max:
        case OP_min:
        case OP_mul:
        case OP_rem:
        case OP_shl:
        case OP_sub:
        case OP_eq:
        case OP_ne:
        case OP_ge:
        case OP_gt:
        case OP_le:
        case OP_lt:
        case OP_cmp:
            return ExtFoldBinary(static_cast<BinaryNode *>(node));
        case OP_select:
            return ExtFoldTernary(static_cast<TernaryNode *>(node));
        default:
            return node;
    }
}

BaseNode *ExtConstantFold::ExtFoldUnary(UnaryNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    result = DispatchFold(node->Opnd(0));
    if (result != node->Opnd(0)) {
        node->SetOpnd(result, 0);
    }
    return node;
}

BaseNode *ExtConstantFold::ExtFoldBinary(BinaryNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    result = DispatchFold(node->Opnd(0));
    if (result != node->Opnd(0)) {
        node->SetOpnd(result, 0);
    }
    result = DispatchFold(node->Opnd(1));
    if (result != node->Opnd(1)) {
        node->SetOpnd(result, 1);
    }
    return node;
}

BaseNode *ExtConstantFold::ExtFoldTernary(TernaryNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    result = DispatchFold(node->Opnd(kFirstOpnd));
    if (result != node->Opnd(kFirstOpnd)) {
        node->SetOpnd(result, kFirstOpnd);
    }
    result = DispatchFold(node->Opnd(kSecondOpnd));
    if (result != node->Opnd(kSecondOpnd)) {
        node->SetOpnd(result, kSecondOpnd);
    }
    result = DispatchFold(node->Opnd(kThirdOpnd));
    if (result != node->Opnd(kThirdOpnd)) {
        node->SetOpnd(result, kThirdOpnd);
    }
    return node;
}

BaseNode *ExtConstantFold::ExtFold(BaseNode *node)
{
    if (node == nullptr || kOpcodeInfo.IsStmt(node->GetOpCode())) {
        return nullptr;
    }
    return DispatchFold(node);
}

BaseNode *ExtConstantFold::ExtFoldIor(BinaryNode *node)
{
    CHECK_NULL_FATAL(node);
    // The target pattern (Cior, Lior):
    // x == c || x == c+1 || ... || x == c+k
    // ==> le (x - c), k
    // where c is int. i
    // Leave other cases for future including extended simplification of partial expressions
    std::queue<BaseNode *> operands;
    std::vector<int64> uniqOperands;
    operands.push(node);
    int64 minVal = LLONG_MAX;
    bool isWorkable = true;
    BaseNode *lNode = nullptr;

    while (!operands.empty()) {
        BaseNode *operand = operands.front();
        operands.pop();
        Opcode op = operand->GetOpCode();
        if (op == OP_cior || op == OP_lior) {
            operands.push(static_cast<BinaryNode *>(operand)->GetBOpnd(0));
            operands.push(static_cast<BinaryNode *>(operand)->GetBOpnd(1));
        } else if (op == OP_eq) {
            BinaryNode *bNode = static_cast<BinaryNode *>(operand);
            if (lNode == nullptr) {
                if (bNode->Opnd(0)->GetOpCode() == OP_dread || bNode->Opnd(0)->GetOpCode() == OP_iread) {
                    lNode = bNode->Opnd(0);
                } else {
                    // Consider other cases in future
                    isWorkable = false;
                    break;
                }
            }

            if ((lNode->IsSameContent(bNode->Opnd(0))) && (bNode->Opnd(1)->GetOpCode() == OP_constval) &&
                (IsPrimitiveInteger(bNode->Opnd(1)->GetPrimType()))) {
                MIRConst *rConstVal = safe_cast<ConstvalNode>(bNode->Opnd(1))->GetConstVal();
                int64 rVal = static_cast<MIRIntConst *>(rConstVal)->GetExtValue();
                minVal = std::min(minVal, rVal);
                uniqOperands.push_back(rVal);
            } else {
                isWorkable = false;
                break;
            }
        } else {
            isWorkable = false;
            break;
        }
    }

    if (isWorkable) {
        std::sort(uniqOperands.begin(), uniqOperands.end());
        uniqOperands.erase(std::unique(uniqOperands.begin(), uniqOperands.end()), uniqOperands.end());
        if ((uniqOperands.size() >= 2) && // operand count is not less than 2
            (uniqOperands[uniqOperands.size() - 1] == uniqOperands[0] + static_cast<int64>(uniqOperands.size()) - 1)) {
            PrimType nPrimType = node->GetPrimType();
            BaseNode *diffVal;
            ConstvalNode *lowVal = mirModule->GetMIRBuilder()->CreateIntConst(minVal, nPrimType);
            diffVal = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_sub, nPrimType, lNode, lowVal);
            PrimType cmpPrimType = (nPrimType == PTY_i64 || nPrimType == PTY_u64) ? PTY_u64 : PTY_u32;
            MIRType *cmpMirType = (nPrimType == PTY_i64 || nPrimType == PTY_u64)
                                      ? GlobalTables::GetTypeTable().GetUInt64()
                                      : GlobalTables::GetTypeTable().GetUInt32();
            ConstvalNode *deltaVal =
                mirModule->GetMIRBuilder()->CreateIntConst(static_cast<int64>(uniqOperands.size()) - 1, cmpPrimType);
            CompareNode *result;
            result = mirModule->GetMIRBuilder()->CreateExprCompare(OP_le, *cmpMirType, *cmpMirType, diffVal, deltaVal);
            return result;
        } else {
            return node;
        }
    } else {
        return node;
    }
}

BaseNode *ExtConstantFold::ExtFoldXand(BinaryNode *node)
{
    // The target pattern (Cand, Land):
    // (x & m1) == c1 && (x & m2) == c2 && ... && (x & Mk) == ck
    // where mi and ci shall be all int constants
    // ==> (x & M) == C

    CHECK_NULL_FATAL(node);
    CHECK_FATAL(node->GetOpCode() == OP_cand || node->GetOpCode() == OP_land,
                "Operator is neither OP_cand nor OP_land");

    BaseNode *lnode = DispatchFold(node->Opnd(0));
    if (lnode != node->Opnd(0)) {
        node->SetOpnd(lnode, 0);
    }

    BaseNode *rnode = DispatchFold(node->Opnd(1));
    if (rnode != node->Opnd(1)) {
        node->SetOpnd(rnode, 1);
    }

    // Check if it is of the form of (x & m) == c cand (x & m') == c'
    ASSERT_NOT_NULL(rnode);
    if ((lnode->GetOpCode() == OP_eq) && (rnode->GetOpCode() == OP_eq) && (lnode->Opnd(0)->GetOpCode() == OP_band) &&
        (lnode->Opnd(0)->Opnd(1)->GetOpCode() == OP_constval) &&
        (IsPrimitiveInteger(lnode->Opnd(0)->Opnd(1)->GetPrimType())) && (lnode->Opnd(1)->GetOpCode() == OP_constval) &&
        (IsPrimitiveInteger(lnode->Opnd(1)->GetPrimType())) && (rnode->Opnd(0)->GetOpCode() == OP_band) &&
        (rnode->Opnd(0)->Opnd(1)->GetOpCode() == OP_constval) &&
        (IsPrimitiveInteger(rnode->Opnd(0)->Opnd(1)->GetPrimType())) && (rnode->Opnd(1)->GetOpCode() == OP_constval) &&
        (IsPrimitiveInteger(rnode->Opnd(1)->GetPrimType())) &&
        (lnode->Opnd(0)->Opnd(0)->IsSameContent(rnode->Opnd(0)->Opnd(0)))) {
        MIRConst *lmConstVal = safe_cast<ConstvalNode>(lnode->Opnd(0)->Opnd(1))->GetConstVal();
        uint64 lmVal = static_cast<uint64>(static_cast<MIRIntConst *>(lmConstVal)->GetExtValue());
        MIRConst *rmConstVal = safe_cast<ConstvalNode>(rnode->Opnd(0)->Opnd(1))->GetConstVal();
        uint64 rmVal = static_cast<uint64>(static_cast<MIRIntConst *>(rmConstVal)->GetExtValue());
        MIRConst *lcConstVal = safe_cast<ConstvalNode>(lnode->Opnd(1))->GetConstVal();
        uint64 lcVal = static_cast<uint64>(static_cast<MIRIntConst *>(lcConstVal)->GetExtValue());
        MIRConst *rcConstVal = safe_cast<ConstvalNode>(rnode->Opnd(1))->GetConstVal();
        uint64 rcVal = static_cast<uint64>(static_cast<MIRIntConst *>(rcConstVal)->GetExtValue());

        bool isWorkable = true;
        for (uint64 i = 0; i < k64BitSize; i++) {
            if ((lmVal & (1UL << i)) == (rmVal & (1UL << i)) && (lcVal & (1UL << i)) != (rcVal & (1UL << i))) {
                isWorkable = false;
                break;
            }
        }

        if (isWorkable) {
            uint64 mVal = lmVal | rmVal;
            uint64 cVal = lcVal | rcVal;
            PrimType mPrimType = lnode->Opnd(0)->Opnd(1)->GetPrimType();
            ConstvalNode *mIntConst = mirModule->GetMIRBuilder()->CreateIntConst(static_cast<int64>(mVal), mPrimType);
            PrimType cPrimType = lnode->Opnd(1)->GetPrimType();
            ConstvalNode *cIntConst = mirModule->GetMIRBuilder()->CreateIntConst(static_cast<int64>(cVal), cPrimType);
            BinaryNode *eqNode = static_cast<BinaryNode *>(lnode);
            BinaryNode *bandNode = static_cast<BinaryNode *>(eqNode->Opnd(0));
            bandNode->SetOpnd(mIntConst, 1);
            eqNode->SetOpnd(cIntConst, 1);
            return eqNode;
        }
    }
    return node;
}

StmtNode *ExtConstantFold::ExtSimplifyBlock(BlockNode *node)
{
    CHECK_NULL_FATAL(node);
    if (node->GetFirst() == nullptr) {
        return node;
    }
    StmtNode *s = node->GetFirst();
    do {
        (void)ExtSimplify(s);
        s = s->GetNext();
        ;
    } while (s != nullptr);
    return node;
}

StmtNode *ExtConstantFold::ExtSimplifyIf(IfStmtNode *node)
{
    CHECK_NULL_FATAL(node);
    (void)ExtSimplify(node->GetThenPart());
    if (node->GetElsePart()) {
        (void)ExtSimplify(node->GetElsePart());
    }
    BaseNode *origTest = node->Opnd();
    BaseNode *returnValue = ExtFold(node->Opnd());
    if (returnValue != origTest) {
        node->SetOpnd(returnValue, 0);
    }
    return node;
}

StmtNode *ExtConstantFold::ExtSimplifyDassign(DassignNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *returnValue;
    returnValue = ExtFold(node->GetRHS());
    if (returnValue != node->GetRHS()) {
        node->SetRHS(returnValue);
    }
    return node;
}

StmtNode *ExtConstantFold::ExtSimplifyIassign(IassignNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *returnValue;
    returnValue = ExtFold(node->GetRHS());
    if (returnValue != node->GetRHS()) {
        node->SetRHS(returnValue);
    }
    return node;
}

StmtNode *ExtConstantFold::ExtSimplifyWhile(WhileStmtNode *node)
{
    CHECK_NULL_FATAL(node);
    if (node->Opnd(0) == nullptr) {
        return node;
    }
    BaseNode *returnValue = ExtFold(node->Opnd(0));
    if (returnValue != node->Opnd(0)) {
        node->SetOpnd(returnValue, 0);
    }
    return node;
}
}  // namespace maple
