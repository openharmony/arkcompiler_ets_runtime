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
        case OP_cand:
        case OP_land:
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
