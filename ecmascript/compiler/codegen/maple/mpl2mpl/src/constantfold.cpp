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

#include "constantfold.h"
#include <cmath>
#include <cfloat>
#include <climits>
#include <type_traits>
#include "mpl_logging.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "global_tables.h"
#include "me_option.h"
#include "maple_phase_manager.h"
#include "mir_type.h"

namespace maple {

namespace {
constexpr uint32 kByteSizeOfBit64 = 8;                            // byte number for 64 bit
constexpr uint32 kBitSizePerByte = 8;
constexpr maple::int32 kMaxOffset = INT_MAX - 8;

enum CompareRes : int64 { kLess = -1, kEqual = 0, kGreater = 1 };

std::optional<IntVal> operator*(const std::optional<IntVal> &v1, const std::optional<IntVal> &v2)
{
    if (!v1 && !v2) {
        return std::nullopt;
    }

    // Perform all calculations in terms of the maximum available signed type.
    // The value will be truncated for an appropriate type when constant is created in PairToExpr function
    return v1 && v2 ? v1->Mul(*v2, PTY_i64) : IntVal(static_cast<uint64>(0), PTY_i64);
}

// Perform all calculations in terms of the maximum available signed type.
// The value will be truncated for an appropriate type when constant is created in PairToExpr function
std::optional<IntVal> AddSub(const std::optional<IntVal> &v1, const std::optional<IntVal> &v2, bool isAdd)
{
    if (!v1 && !v2) {
        return std::nullopt;
    }

    if (v1 && v2) {
        return isAdd ? v1->Add(*v2, PTY_i64) : v1->Sub(*v2, PTY_i64);
    }

    if (v1) {
        return v1->TruncOrExtend(PTY_i64);
    }

    // !v1 && v2
    return isAdd ? v2->TruncOrExtend(PTY_i64) : -(v2->TruncOrExtend(PTY_i64));
}

std::optional<IntVal> operator+(const std::optional<IntVal> &v1, const std::optional<IntVal> &v2)
{
    return AddSub(v1, v2, true);
}

std::optional<IntVal> operator-(const std::optional<IntVal> &v1, const std::optional<IntVal> &v2)
{
    return AddSub(v1, v2, false);
}

}  // anonymous namespace

// This phase is designed to achieve compiler optimization by
// simplifying constant expressions. The constant expression
// is evaluated and replaced by the value calculated on compile
// time to save time on runtime.
//
// The main procedure shows as following:
// A. Analyze expression type
// B. Analysis operator type
// C. Replace the expression with the result of the operation

// true if the constant's bits are made of only one group of contiguous 1's
// starting at bit 0
static bool ContiguousBitsOf1(uint64 x)
{
    if (x == 0) {
        return false;
    }
    return (~x & (x + 1)) == (x + 1);
}

inline bool IsPowerOf2(uint64 num)
{
    if (num == 0) {
        return false;
    }
    return (~(num - 1) & num) == num;
}

BinaryNode *ConstantFold::NewBinaryNode(BinaryNode *old, Opcode op, PrimType primType, BaseNode *lhs,
                                        BaseNode *rhs) const
{
    CHECK_NULL_FATAL(old);
    BinaryNode *result = nullptr;
    if (old->GetOpCode() == op && old->GetPrimType() == primType && old->Opnd(0) == lhs && old->Opnd(1) == rhs) {
        result = old;
    } else {
        result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(op, primType, lhs, rhs);
    }
    return result;
}

UnaryNode *ConstantFold::NewUnaryNode(UnaryNode *old, Opcode op, PrimType primType, BaseNode *expr) const
{
    CHECK_NULL_FATAL(old);
    UnaryNode *result = nullptr;
    if (old->GetOpCode() == op && old->GetPrimType() == primType && old->Opnd(0) == expr) {
        result = old;
    } else {
        result = mirModule->CurFuncCodeMemPool()->New<UnaryNode>(op, primType, expr);
    }
    return result;
}

BaseNode *ConstantFold::PairToExpr(PrimType resultType, const std::pair<BaseNode*, std::optional<IntVal>> &pair) const
{
    CHECK_NULL_FATAL(pair.first);
    BaseNode *result = pair.first;
    if (!pair.second || *pair.second == 0 || GetPrimTypeSize(resultType) > k8ByteSize) {
        return result;
    }
    if (pair.first->GetOpCode() == OP_neg && !pair.second->GetSignBit()) {
        // -a, 5 -> 5 - a
        ConstvalNode *val = mirModule->GetMIRBuilder()->CreateIntConst(
            static_cast<uint64>(pair.second->GetExtValue()), resultType);
        BaseNode *r = static_cast<UnaryNode*>(pair.first)->Opnd(0);
        result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_sub, resultType, val, r);
    } else {
        if ((!pair.second->GetSignBit() &&
            pair.second->GetSXTValue(static_cast<uint8>(GetPrimTypeBitSize(resultType))) > 0) ||
            pair.second->TruncOrExtend(resultType).IsMinValue() ||
            pair.second->GetSXTValue() == INT64_MIN) {
            // +-a, 5 -> a + 5
            ConstvalNode *val = mirModule->GetMIRBuilder()->CreateIntConst(
                static_cast<uint64>(pair.second->GetExtValue()), resultType);
            result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_add, resultType, pair.first, val);
        } else {
            // +-a, -5 -> a + -5
            ConstvalNode *val = mirModule->GetMIRBuilder()->CreateIntConst(
                static_cast<uint64>((-pair.second.value()).GetExtValue()), resultType);
            result = mirModule->CurFuncCodeMemPool()->New<BinaryNode>(OP_sub, resultType, pair.first, val);
        }
    }
    return result;
}

std::pair<BaseNode *, std::optional<IntVal>> ConstantFold::FoldBase(BaseNode *node) const
{
    return std::make_pair(node, std::nullopt);
}

std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::DispatchFold(BaseNode *node)
{
    CHECK_NULL_FATAL(node);
    if (GetPrimTypeSize(node->GetPrimType()) > k8ByteSize) {
        return {node, std::nullopt};
    }
    switch (node->GetOpCode()) {
        case OP_abs:
        case OP_bnot:
        case OP_lnot:
        case OP_neg:
        case OP_sqrt:
            return FoldUnary(static_cast<UnaryNode*>(node));
        case OP_ceil:
        case OP_floor:
        case OP_trunc:
        case OP_cvt:
            return FoldTypeCvt(static_cast<TypeCvtNode*>(node));
        case OP_sext:
        case OP_zext:
        case OP_extractbits:
            return FoldExtractbits(static_cast<ExtractbitsNode*>(node));
        case OP_iread:
            return FoldIread(static_cast<IreadNode*>(node));
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
            return FoldBinary(static_cast<BinaryNode*>(node));
        case OP_eq:
        case OP_ne:
        case OP_ge:
        case OP_gt:
        case OP_le:
        case OP_lt:
        case OP_cmp:
            return FoldCompare(static_cast<CompareNode*>(node));
        case OP_retype:
            return FoldRetype(static_cast<RetypeNode*>(node));
        default:
            return FoldBase(static_cast<BaseNode*>(node));
    }
}

BaseNode *ConstantFold::Negate(BaseNode *node) const
{
    CHECK_NULL_FATAL(node);
    return mirModule->CurFuncCodeMemPool()->New<UnaryNode>(OP_neg, PrimType(node->GetPrimType()), node);
}

BaseNode *ConstantFold::Negate(UnaryNode *node) const
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    if (node->GetOpCode() == OP_neg) {
        result = static_cast<BaseNode*>(node->Opnd(0));
    } else {
        BaseNode *n = static_cast<BaseNode*>(node);
        result = NewUnaryNode(node, OP_neg, node->GetPrimType(), n);
    }
    return result;
}

BaseNode *ConstantFold::Negate(const ConstvalNode *node) const
{
    CHECK_NULL_FATAL(node);
    ConstvalNode *copy = node->CloneTree(mirModule->GetCurFuncCodeMPAllocator());
    CHECK_NULL_FATAL(copy);
    copy->GetConstVal()->Neg();
    return copy;
}

BaseNode *ConstantFold::NegateTree(BaseNode *node) const
{
    CHECK_NULL_FATAL(node);
    if (node->IsUnaryNode()) {
        return Negate(static_cast<UnaryNode*>(node));
    } else if (node->GetOpCode() == OP_constval) {
        return Negate(static_cast<ConstvalNode*>(node));
    } else {
        return Negate(static_cast<BaseNode*>(node));
    }
}

MIRIntConst *ConstantFold::FoldIntConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                                          const MIRIntConst &intConst0,
                                                          const MIRIntConst &intConst1) const
{
    uint64 result = 0;

    bool greater = intConst0.GetValue().Greater(intConst1.GetValue(), opndType);
    bool equal = intConst0.GetValue().Equal(intConst1.GetValue(), opndType);
    bool less = intConst0.GetValue().Less(intConst1.GetValue(), opndType);

    switch (opcode) {
        case OP_eq: {
            result = equal;
            break;
        }
        case OP_ge: {
            result = (greater || equal) ? 1 : 0;
            break;
        }
        case OP_gt: {
            result = greater;
            break;
        }
        case OP_le: {
            result = (less || equal) ? 1 : 0;
            break;
        }
        case OP_lt: {
            result = less;
            break;
        }
        case OP_ne: {
            result = !equal;
            break;
        }
        case OP_cmp: {
            if (greater) {
                result = kGreater;
            } else if (equal) {
                result = kEqual;
            } else if (less) {
                result = static_cast<uint64>(kLess);
            }
            break;
        }
        default:
            DEBUG_ASSERT(false, "Unknown opcode for FoldIntConstComparison");
            break;
    }
    // determine the type
    MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
    // form the constant
    MIRIntConst *constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(result, type);
    return constValue;
}

ConstvalNode *ConstantFold::FoldIntConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                                   const ConstvalNode &const0, const ConstvalNode &const1) const
{
    const MIRIntConst *intConst0 = safe_cast<MIRIntConst>(const0.GetConstVal());
    const MIRIntConst *intConst1 = safe_cast<MIRIntConst>(const1.GetConstVal());
    CHECK_NULL_FATAL(intConst0);
    CHECK_NULL_FATAL(intConst1);
    MIRIntConst *constValue = FoldIntConstComparisonMIRConst(opcode, resultType, opndType, *intConst0, *intConst1);
    // form the ConstvalNode
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(resultType);
    resultConst->SetConstVal(constValue);
    return resultConst;
}

MIRConst *ConstantFold::FoldIntConstBinaryMIRConst(Opcode opcode, PrimType resultType, const MIRIntConst &intConst0,
                                                   const MIRIntConst &intConst1)
{
    IntVal intVal0 = intConst0.GetValue();
    IntVal intVal1 = intConst1.GetValue();
    IntVal result(static_cast<uint64>(0), resultType);

    switch (opcode) {
        case OP_add: {
            result = intVal0.Add(intVal1, resultType);
            break;
        }
        case OP_sub: {
            result = intVal0.Sub(intVal1, resultType);
            break;
        }
        case OP_mul: {
            result = intVal0.Mul(intVal1, resultType);
            break;
        }
        case OP_div: {
            result = intVal0.Div(intVal1, resultType);
            break;
        }
        case OP_rem: {
            result = intVal0.Rem(intVal1, resultType);
            break;
        }
        case OP_ashr: {
            result = intVal0.AShr(intVal1.GetZXTValue() % GetAlignedPrimTypeBitSize(resultType), resultType);
            break;
        }
        case OP_lshr: {
            result = intVal0.LShr(intVal1.GetZXTValue() % GetAlignedPrimTypeBitSize(resultType), resultType);
            break;
        }
        case OP_shl: {
            result = intVal0.Shl(intVal1.GetZXTValue() % GetAlignedPrimTypeBitSize(resultType), resultType);
            break;
        }
        case OP_max: {
            result = Max(intVal0, intVal1, resultType);
            break;
        }
        case OP_min: {
            result = Min(intVal0, intVal1, resultType);
            break;
        }
        case OP_band: {
            result = intVal0.And(intVal1, resultType);
            break;
        }
        case OP_bior: {
            result = intVal0.Or(intVal1, resultType);
            break;
        }
        case OP_bxor: {
            result = intVal0.Xor(intVal1, resultType);
            break;
        }
        default:
            DEBUG_ASSERT(false, "Unknown opcode for FoldIntConstBinary");
            break;
    }
    // determine the type
    MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
    // form the constant
    MIRIntConst *constValue =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(result.GetExtValue()), type);
    return constValue;
}

ConstvalNode *ConstantFold::FoldIntConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                               const ConstvalNode &const1) const
{
    const MIRIntConst *intConst0 = safe_cast<MIRIntConst>(const0.GetConstVal());
    const MIRIntConst *intConst1 = safe_cast<MIRIntConst>(const1.GetConstVal());
    CHECK_NULL_FATAL(intConst0);
    CHECK_NULL_FATAL(intConst1);
    MIRConst *constValue = FoldIntConstBinaryMIRConst(opcode, resultType, *intConst0, *intConst1);
    // form the ConstvalNode
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(resultType);
    resultConst->SetConstVal(constValue);
    return resultConst;
}

ConstvalNode *ConstantFold::FoldFPConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                              const ConstvalNode &const1) const
{
    DEBUG_ASSERT(const0.GetPrimType() == const1.GetPrimType(), "The types of the operands must match");
    const MIRDoubleConst *doubleConst0 = nullptr;
    const MIRDoubleConst *doubleConst1 = nullptr;
    const MIRFloatConst *floatConst0 = nullptr;
    const MIRFloatConst *floatConst1 = nullptr;
    bool useDouble = (const0.GetPrimType() == PTY_f64);
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(resultType);
    if (useDouble) {
        doubleConst0 = safe_cast<MIRDoubleConst>(const0.GetConstVal());
        doubleConst1 = safe_cast<MIRDoubleConst>(const1.GetConstVal());
        CHECK_NULL_FATAL(doubleConst0);
        CHECK_NULL_FATAL(doubleConst1);
    } else {
        floatConst0 = safe_cast<MIRFloatConst>(const0.GetConstVal());
        floatConst1 = safe_cast<MIRFloatConst>(const1.GetConstVal());
        CHECK_NULL_FATAL(floatConst0);
        CHECK_NULL_FATAL(floatConst1);
    }
    float constValueFloat = 0.0;
    double constValueDouble = 0.0;
    switch (opcode) {
        case OP_add: {
            if (useDouble) {
                constValueDouble = doubleConst0->GetValue() + doubleConst1->GetValue();
            } else {
                constValueFloat = floatConst0->GetValue() + floatConst1->GetValue();
            }
            break;
        }
        case OP_sub: {
            if (useDouble) {
                constValueDouble = doubleConst0->GetValue() - doubleConst1->GetValue();
            } else {
                constValueFloat = floatConst0->GetValue() - floatConst1->GetValue();
            }
            break;
        }
        case OP_mul: {
            if (useDouble) {
                constValueDouble = doubleConst0->GetValue() * doubleConst1->GetValue();
            } else {
                constValueFloat = floatConst0->GetValue() * floatConst1->GetValue();
            }
            break;
        }
        case OP_div: {
            // for floats div by 0 is well defined
            if (useDouble) {
                constValueDouble = doubleConst0->GetValue() / doubleConst1->GetValue();
            } else {
                constValueFloat = floatConst0->GetValue() / floatConst1->GetValue();
            }
            break;
        }
        case OP_max: {
            if (useDouble) {
                constValueDouble = (doubleConst0->GetValue() >= doubleConst1->GetValue()) ? doubleConst0->GetValue()
                                                                                        : doubleConst1->GetValue();
            } else {
                constValueFloat = (floatConst0->GetValue() >= floatConst1->GetValue()) ? floatConst0->GetValue()
                                                                                    : floatConst1->GetValue();
            }
            break;
        }
        case OP_min: {
            if (useDouble) {
                constValueDouble = (doubleConst0->GetValue() <= doubleConst1->GetValue()) ? doubleConst0->GetValue()
                                                                                        : doubleConst1->GetValue();
            } else {
                constValueFloat = (floatConst0->GetValue() <= floatConst1->GetValue()) ? floatConst0->GetValue()
                                                                                    : floatConst1->GetValue();
            }
            break;
        }
        case OP_rem:
        case OP_ashr:
        case OP_lshr:
        case OP_shl:
        case OP_band:
        case OP_bior:
        case OP_bxor: {
            DEBUG_ASSERT(false, "Unexpected opcode in FoldFPConstBinary");
            break;
        }
        default:
            DEBUG_ASSERT(false, "Unknown opcode for FoldFPConstBinary");
            break;
    }
    if (resultType == PTY_f64) {
        resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(constValueDouble));
    } else {
        resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateFloatConst(constValueFloat));
    }
    return resultConst;
}

bool ConstantFold::ConstValueEqual(int64 leftValue, int64 rightValue) const
{
    return (leftValue == rightValue);
}

bool ConstantFold::ConstValueEqual(float leftValue, float rightValue) const
{
    auto result = fabs(leftValue - rightValue);
    return leftValue <= FLT_MIN && rightValue <= FLT_MIN ? result < FLT_MIN : result <= FLT_MIN;
}

bool ConstantFold::ConstValueEqual(double leftValue, double rightValue) const
{
    auto result = fabs(leftValue - rightValue);
    return leftValue <= DBL_MIN && rightValue <= DBL_MIN ? result < DBL_MIN : result <= DBL_MIN;
}

template<typename T>
bool ConstantFold::FullyEqual(T leftValue, T rightValue) const
{
    if (std::isinf(leftValue) && std::isinf(rightValue)) {
        // (inf == inf), add the judgement here in case of the subtraction between float type inf
        return true;
    } else {
        return ConstValueEqual(leftValue, rightValue);
    }
}

template<typename T>
int64 ConstantFold::ComparisonResult(Opcode op, T *leftConst, T *rightConst) const
{
    DEBUG_ASSERT(leftConst != nullptr, "leftConst should not be nullptr");
    typename T::value_type leftValue = leftConst->GetValue();
    DEBUG_ASSERT(rightConst != nullptr, "rightConst should not be nullptr");
    typename T::value_type rightValue = rightConst->GetValue();
    int64 result = 0;
    switch (op) {
        case OP_eq: {
            result = FullyEqual(leftValue, rightValue);
            break;
        }
        case OP_ge: {
            result = (leftValue > rightValue) || FullyEqual(leftValue, rightValue);
            break;
        }
        case OP_gt: {
            result = (leftValue > rightValue);
            break;
        }
        case OP_le: {
            result = (leftValue < rightValue) || FullyEqual(leftValue, rightValue);
            break;
        }
        case OP_lt: {
            result = (leftValue < rightValue);
            break;
        }
        case OP_ne: {
            result = !FullyEqual(leftValue, rightValue);
            break;
        }
        [[clang::fallthrough]];
        case OP_cmp: {
            if (leftValue > rightValue) {
                result = kGreater;
            } else if (FullyEqual(leftValue, rightValue)) {
                result = kEqual;
            } else {
                result = kLess;
            }
            break;
        }
        default:
            DEBUG_ASSERT(false, "Unknown opcode for Comparison");
            break;
    }
    return result;
}

MIRIntConst *ConstantFold::FoldFPConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                                         const MIRConst &leftConst, const MIRConst &rightConst) const
{
    int64 result = 0;
    bool useDouble = (opndType == PTY_f64);
    if (useDouble) {
        result =
            ComparisonResult(opcode, safe_cast<MIRDoubleConst>(&leftConst), safe_cast<MIRDoubleConst>(&rightConst));
    } else {
        result = ComparisonResult(opcode, safe_cast<MIRFloatConst>(&leftConst), safe_cast<MIRFloatConst>(&rightConst));
    }
    MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
    MIRIntConst *resultConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(result), type);
    return resultConst;
}

ConstvalNode *ConstantFold::FoldFPConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                                  const ConstvalNode &const0, const ConstvalNode &const1) const
{
    DEBUG_ASSERT(const0.GetPrimType() == const1.GetPrimType(), "The types of the operands must match");
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(resultType);
    resultConst->SetConstVal(
        FoldFPConstComparisonMIRConst(opcode, resultType, opndType, *const0.GetConstVal(), *const1.GetConstVal()));
    return resultConst;
}

MIRConst *ConstantFold::FoldConstComparisonMIRConst(Opcode opcode, PrimType resultType, PrimType opndType,
                                                    const MIRConst &const0, const MIRConst &const1) const
{
    MIRConst *returnValue = nullptr;
    if (IsPrimitiveInteger(opndType)) {
        const auto *intConst0 = safe_cast<MIRIntConst>(&const0);
        const auto *intConst1 = safe_cast<MIRIntConst>(&const1);
        ASSERT_NOT_NULL(intConst0);
        ASSERT_NOT_NULL(intConst1);
        returnValue = FoldIntConstComparisonMIRConst(opcode, resultType, opndType, *intConst0, *intConst1);
    } else if (opndType == PTY_f32 || opndType == PTY_f64) {
        returnValue = FoldFPConstComparisonMIRConst(opcode, resultType, opndType, const0, const1);
    } else {
        DEBUG_ASSERT(false, "Unhandled case for FoldConstComparisonMIRConst");
    }
    return returnValue;
}

ConstvalNode *ConstantFold::FoldConstComparison(Opcode opcode, PrimType resultType, PrimType opndType,
                                                const ConstvalNode &const0, const ConstvalNode &const1) const
{
    ConstvalNode *returnValue = nullptr;
    if (IsPrimitiveInteger(opndType)) {
        returnValue = FoldIntConstComparison(opcode, resultType, opndType, const0, const1);
    } else if (opndType == PTY_f32 || opndType == PTY_f64) {
        returnValue = FoldFPConstComparison(opcode, resultType, opndType, const0, const1);
    } else {
        DEBUG_ASSERT(false, "Unhandled case for FoldConstComparison");
    }
    return returnValue;
}

CompareNode *ConstantFold::FoldConstComparisonReverse(Opcode opcode, PrimType resultType, PrimType opndType,
                                                      BaseNode &l, BaseNode &r) const
{
    CompareNode *result = nullptr;
    Opcode op = opcode;
    switch (opcode) {
        case OP_gt: {
            op = OP_lt;
            break;
        }
        case OP_lt: {
            op = OP_gt;
            break;
        }
        case OP_ge: {
            op = OP_le;
            break;
        }
        case OP_le: {
            op = OP_ge;
            break;
        }
        case OP_eq: {
            break;
        }
        case OP_ne: {
            break;
        }
        default:
            DEBUG_ASSERT(false, "Unknown opcode for FoldConstComparisonReverse");
            break;
    }

    result =
        mirModule->CurFuncCodeMemPool()->New<CompareNode>(Opcode(op), PrimType(resultType), PrimType(opndType), &r, &l);
    return result;
}

ConstvalNode *ConstantFold::FoldConstBinary(Opcode opcode, PrimType resultType, const ConstvalNode &const0,
                                            const ConstvalNode &const1) const
{
    ConstvalNode *returnValue = nullptr;
    if (IsPrimitiveInteger(resultType)) {
        returnValue = FoldIntConstBinary(opcode, resultType, const0, const1);
    } else if (resultType == PTY_f32 || resultType == PTY_f64) {
        returnValue = FoldFPConstBinary(opcode, resultType, const0, const1);
    } else {
        DEBUG_ASSERT(false, "Unhandled case for FoldConstBinary");
    }
    return returnValue;
}

MIRIntConst *ConstantFold::FoldIntConstUnaryMIRConst(Opcode opcode, PrimType resultType, const MIRIntConst *constNode)
{
    CHECK_NULL_FATAL(constNode);
    IntVal result = constNode->GetValue().TruncOrExtend(resultType);
    switch (opcode) {
        case OP_abs: {
            if (IsSignedInteger(constNode->GetType().GetPrimType()) && result.GetSignBit()) {
                result = -result;
            }
            break;
        }
        case OP_bnot: {
            result = ~result;
            break;
        }
        case OP_lnot: {
            uint64 resultInt = result == 0 ? 1 : 0;
            result = {resultInt, resultType};
            break;
        }
        case OP_neg: {
            result = -result;
            break;
        }
        case OP_sext:         // handled in FoldExtractbits
        case OP_zext:         // handled in FoldExtractbits
        case OP_extractbits:  // handled in FoldExtractbits
        case OP_sqrt: {
            DEBUG_ASSERT(false, "Unexpected opcode in FoldIntConstUnaryMIRConst");
            break;
        }
        default:
            DEBUG_ASSERT(false, "Unknown opcode for FoldIntConstUnaryMIRConst");
            break;
    }
    // determine the type
    MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
    // form the constant
    MIRIntConst *constValue =
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(result.GetExtValue()), type);
    return constValue;
}

template <typename T>
ConstvalNode *ConstantFold::FoldFPConstUnary(Opcode opcode, PrimType resultType, ConstvalNode *constNode) const
{
    CHECK_NULL_FATAL(constNode);
    double constValue = 0;
    T *fpCst = static_cast<T*>(constNode->GetConstVal());
    switch (opcode) {
        case OP_neg: {
            constValue = typename T::value_type(-fpCst->GetValue());
            break;
        }
        case OP_abs: {
            constValue = typename T::value_type(fabs(fpCst->GetValue()));
            break;
        }
        case OP_sqrt: {
            constValue = typename T::value_type(sqrt(fpCst->GetValue()));
            break;
        }
        case OP_bnot:
        case OP_lnot:
        case OP_sext:
        case OP_zext:
        case OP_extractbits: {
            DEBUG_ASSERT(false, "Unexpected opcode in FoldFPConstUnary");
            break;
        }
        default:
            DEBUG_ASSERT(false, "Unknown opcode for FoldFPConstUnary");
            break;
    }
    auto *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(resultType);
    if (resultType == PTY_f32) {
        resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateFloatConst(static_cast<float>(constValue)));
    } else if (resultType == PTY_f64) {
        resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(constValue));
    } else {
        CHECK_FATAL(false, "PrimType for MIRFloatConst / MIRDoubleConst should be PTY_f32 / PTY_f64");
    }
    return resultConst;
}

ConstvalNode *ConstantFold::FoldConstUnary(Opcode opcode, PrimType resultType, ConstvalNode &constNode) const
{
    ConstvalNode *returnValue = nullptr;
    if (IsPrimitiveInteger(resultType)) {
        const MIRIntConst *cst = safe_cast<MIRIntConst>(constNode.GetConstVal());
        auto constValue = FoldIntConstUnaryMIRConst(opcode, resultType, cst);
        returnValue = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
        returnValue->SetPrimType(resultType);
        returnValue->SetConstVal(constValue);
    } else if (resultType == PTY_f32) {
        returnValue = FoldFPConstUnary<MIRFloatConst>(opcode, resultType, &constNode);
    } else if (resultType == PTY_f64) {
        returnValue = FoldFPConstUnary<MIRDoubleConst>(opcode, resultType, &constNode);
    } else {
        DEBUG_ASSERT(false, "Unhandled case for FoldConstUnary");
    }
    return returnValue;
}

std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::FoldRetype(RetypeNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = node;
    std::pair<BaseNode*, std::optional<IntVal>> p = DispatchFold(node->Opnd(0));
    if (node->Opnd(0) != p.first) {
        RetypeNode *newRetNode = node->CloneTree(mirModule->GetCurFuncCodeMPAllocator());
        CHECK_FATAL(newRetNode != nullptr, "newRetNode is null in ConstantFold::FoldRetype");
        newRetNode->SetOpnd(PairToExpr(node->Opnd(0)->GetPrimType(), p), 0);
        result = newRetNode;
    }
    return std::make_pair(result, std::nullopt);
}

std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::FoldUnary(UnaryNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    std::optional<IntVal> sum = std::nullopt;
    std::pair<BaseNode*, std::optional<IntVal>> p = DispatchFold(node->Opnd(0));
    ConstvalNode *cst = safe_cast<ConstvalNode>(p.first);
    if (cst != nullptr) {
        result = FoldConstUnary(node->GetOpCode(), node->GetPrimType(), *cst);
    } else {
        bool isInt = IsPrimitiveInteger(node->GetPrimType());
        // The neg node will be recreated regardless of whether the folding is successful or not. And the neg node's
        // primType will be set to opnd type. There will be problems in some cases. For example:
        // before cf:
        //   neg i32 (eq u1 f32 (dread f32 %f_4_2, constval f32 0f))
        // after cf:
        //   neg u1 (eq u1 f32 (dread f32 %f_4_2, constval f32 0f))  # wrong!
        // As a workaround, we exclude u1 opnd type
        if (isInt && node->GetOpCode() == OP_neg && p.first->GetPrimType() != PTY_u1) {
            result = NegateTree(p.first);
            if (result->GetOpCode() == OP_neg) {
                PrimType origPtyp = node->GetPrimType();
                PrimType newPtyp = result->GetPrimType();
                if (newPtyp == origPtyp) {
                if (static_cast<UnaryNode*>(result)->Opnd(0) == node->Opnd(0)) {
                    // NegateTree returned an UnaryNode quivalent to `n`, so keep the
                    // original UnaryNode to preserve identity
                    result = node;
                }
                } else {
                    if (GetPrimTypeSize(newPtyp) != GetPrimTypeSize(origPtyp)) {
                        // do not fold explicit cvt
                        result = NewUnaryNode(node, node->GetOpCode(), node->GetPrimType(),
                            PairToExpr(node->Opnd(0)->GetPrimType(), p));
                        return std::make_pair(result, std::nullopt);
                    } else {
                        result->SetPrimType(origPtyp);
                    }
                }
            }
            if (p.second) {
                sum = -(*p.second);
            }
        } else {
            result =
                NewUnaryNode(node, node->GetOpCode(), node->GetPrimType(), PairToExpr(node->Opnd(0)->GetPrimType(), p));
        }
    }
    return std::make_pair(result, sum);
}

static bool FloatToIntOverflow(float fval, PrimType totype)
{
    static const float safeFloatMaxToInt32 = 2147483520.0f;  // 2^31 - 128
    static const float safeFloatMinToInt32 = -2147483520.0f;
    static const float safeFloatMaxToInt64 = 9223372036854775680.0f;  // 2^63 - 128
    static const float safeFloatMinToInt64 = -9223372036854775680.0f;
    if (!std::isfinite(fval)) {
        return true;
    }
    if (totype == PTY_i64 || totype == PTY_u64) {
        if (fval < safeFloatMinToInt64 || fval > safeFloatMaxToInt64) {
            return true;
        }
    } else {
        if (fval < safeFloatMinToInt32 || fval > safeFloatMaxToInt32) {
            return true;
        }
    }
    return false;
}

static bool DoubleToIntOverflow(double dval, PrimType totype)
{
    static const double safeDoubleMaxToInt32 = 2147482624.0;  // 2^31 - 1024
    static const double safeDoubleMinToInt32 = -2147482624.0;
    static const double safeDoubleMaxToInt64 = 9223372036854774784.0;  // 2^63 - 1024
    static const double safeDoubleMinToInt64 = -9223372036854774784.0;
    if (!std::isfinite(dval)) {
        return true;
    }
    if (totype == PTY_i64 || totype == PTY_u64) {
        if (dval < safeDoubleMinToInt64 || dval > safeDoubleMaxToInt64) {
            return true;
        }
    } else {
        if (dval < safeDoubleMinToInt32 || dval > safeDoubleMaxToInt32) {
            return true;
        }
    }
    return false;
}

ConstvalNode *ConstantFold::FoldCeil(const ConstvalNode &cst, PrimType fromType, PrimType toType) const
{
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(toType);
    MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
    if (fromType == PTY_f32) {
        const MIRFloatConst *constValue = safe_cast<MIRFloatConst>(cst.GetConstVal());
        ASSERT_NOT_NULL(constValue);
        float floatValue = ceil(constValue->GetValue());
        if (IsPrimitiveFloat(toType)) {
            resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue));
        } else if (FloatToIntOverflow(floatValue, toType)) {
            return nullptr;
        } else {
            resultConst->SetConstVal(
                GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(floatValue), resultType));
        }
    } else {
        const MIRDoubleConst *constValue = safe_cast<MIRDoubleConst>(cst.GetConstVal());
        ASSERT_NOT_NULL(constValue);
        double doubleValue = ceil(constValue->GetValue());
        if (IsPrimitiveFloat(toType)) {
            resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue));
        } else if (DoubleToIntOverflow(doubleValue, toType)) {
            return nullptr;
        } else {
            resultConst->SetConstVal(
                GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(doubleValue), resultType));
        }
    }
    return resultConst;
}

template <class T>
T ConstantFold::CalIntValueFromFloatValue(T value, const MIRType &resultType) const
{
    DEBUG_ASSERT(kByteSizeOfBit64 >= resultType.GetSize(), "unsupported type");
    size_t shiftNum = (kByteSizeOfBit64 - resultType.GetSize()) * kBitSizePerByte;
    bool isSigned = IsSignedInteger(resultType.GetPrimType());
    int64 max = (IntVal(std::numeric_limits<int64>::max(), PTY_i64) >> shiftNum).GetExtValue();
    uint64 umax = std::numeric_limits<uint64>::max() >> shiftNum;
    int64 min = isSigned ? (IntVal(std::numeric_limits<int64>::min(), PTY_i64) >> shiftNum).GetExtValue() : 0;
    if (isSigned && (value > max)) {
        return static_cast<T>(max);
    } else if (!isSigned && (value > umax)) {
        return static_cast<T>(umax);
    } else if (value < min) {
        return static_cast<T>(min);
    }
    return value;
}

MIRConst *ConstantFold::FoldFloorMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType, bool isFloor) const
{
    MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
    if (fromType == PTY_f32) {
        const auto &constValue = static_cast<const MIRFloatConst&>(cst);
        float floatValue = constValue.GetValue();
        if (isFloor) {
            floatValue = floor(constValue.GetValue());
        }
        if (IsPrimitiveFloat(toType)) {
            return GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue);
        }
        if (FloatToIntOverflow(floatValue, toType)) {
            return nullptr;
        }
        floatValue = CalIntValueFromFloatValue(floatValue, resultType);
        return GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(floatValue), resultType);
    } else {
        const auto &constValue = static_cast<const MIRDoubleConst&>(cst);
        double doubleValue = constValue.GetValue();
        if (isFloor) {
            doubleValue = floor(constValue.GetValue());
        }
        if (IsPrimitiveFloat(toType)) {
            return GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue);
        }
        if (DoubleToIntOverflow(doubleValue, toType)) {
            return nullptr;
        }
        doubleValue = CalIntValueFromFloatValue(doubleValue, resultType);
        // gcc/clang have bugs convert double to unsigned long, must convert to signed long first;
        return GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(doubleValue), resultType);
    }
}

ConstvalNode *ConstantFold::FoldFloor(const ConstvalNode &cst, PrimType fromType, PrimType toType) const
{
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(toType);
    resultConst->SetConstVal(FoldFloorMIRConst(*cst.GetConstVal(), fromType, toType));
    return resultConst;
}

MIRConst *ConstantFold::FoldRoundMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const
{
    MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
    if (fromType == PTY_f32) {
        const auto &constValue = static_cast<const MIRFloatConst&>(cst);
        float floatValue = round(constValue.GetValue());
        if (FloatToIntOverflow(floatValue, toType)) {
            return nullptr;
        }
        return GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<int64>(floatValue), resultType);
    } else if (fromType == PTY_f64) {
        const auto &constValue = static_cast<const MIRDoubleConst&>(cst);
        double doubleValue = round(constValue.GetValue());
        if (DoubleToIntOverflow(doubleValue, toType)) {
            return nullptr;
        }
        return GlobalTables::GetIntConstTable().GetOrCreateIntConst(
            static_cast<uint64>(static_cast<int64>(doubleValue)), resultType);
    } else if (toType == PTY_f32 && IsPrimitiveInteger(fromType)) {
        const auto &constValue = static_cast<const MIRIntConst&>(cst);
        if (IsSignedInteger(fromType)) {
            int64 fromValue = constValue.GetExtValue();
            float floatValue = round(static_cast<float>(fromValue));
            if (static_cast<int64>(floatValue) == fromValue) {
                return GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue);
            }
        } else {
            uint64 fromValue = static_cast<uint64>(constValue.GetExtValue());
            float floatValue = round(static_cast<float>(fromValue));
            if (static_cast<uint64>(floatValue) == fromValue) {
                return GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue);
            }
        }
    } else if (toType == PTY_f64 && IsPrimitiveInteger(fromType)) {
        const auto &constValue = static_cast<const MIRIntConst&>(cst);
        if (IsSignedInteger(fromType)) {
            int64 fromValue = constValue.GetExtValue();
            double doubleValue = round(static_cast<double>(fromValue));
            if (static_cast<int64>(doubleValue) == fromValue) {
                return GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue);
            }
        } else {
            uint64 fromValue = static_cast<uint64>(constValue.GetExtValue());
            double doubleValue = round(static_cast<double>(fromValue));
            if (static_cast<uint64>(doubleValue) == fromValue) {
                return GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue);
            }
        }
    }
    return nullptr;
}

ConstvalNode *ConstantFold::FoldRound(const ConstvalNode &cst, PrimType fromType, PrimType toType) const
{
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(toType);
    resultConst->SetConstVal(FoldRoundMIRConst(*cst.GetConstVal(), fromType, toType));
    return resultConst;
}

ConstvalNode *ConstantFold::FoldTrunc(const ConstvalNode &cst, PrimType fromType, PrimType toType) const
{
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    resultConst->SetPrimType(toType);
    MIRType &resultType = *GlobalTables::GetTypeTable().GetPrimType(toType);
    if (fromType == PTY_f32) {
        const MIRFloatConst *constValue = safe_cast<MIRFloatConst>(cst.GetConstVal());
        CHECK_NULL_FATAL(constValue);
        float floatValue = trunc(constValue->GetValue());
        if (IsPrimitiveFloat(toType)) {
            resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue));
        } else if (FloatToIntOverflow(floatValue, toType)) {
            return nullptr;
        } else {
            resultConst->SetConstVal(
                GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(floatValue), resultType));
        }
    } else {
        const MIRDoubleConst *constValue = safe_cast<MIRDoubleConst>(cst.GetConstVal());
        CHECK_NULL_FATAL(constValue);
        double doubleValue = trunc(constValue->GetValue());
        if (IsPrimitiveFloat(toType)) {
            resultConst->SetConstVal(GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue));
        } else if (DoubleToIntOverflow(doubleValue, toType)) {
            return nullptr;
        } else {
            resultConst->SetConstVal(
                GlobalTables::GetIntConstTable().GetOrCreateIntConst(static_cast<uint64>(doubleValue), resultType));
        }
    }
    return resultConst;
}

MIRConst *ConstantFold::FoldTypeCvtMIRConst(const MIRConst &cst, PrimType fromType, PrimType toType) const
{
    if (IsPrimitiveInteger(fromType) && IsPrimitiveInteger(toType)) {
        MIRConst *toConst = nullptr;
        uint32 fromSize = GetPrimTypeBitSize(fromType);
        uint32 toSize = GetPrimTypeBitSize(toType);
        // GetPrimTypeBitSize(PTY_u1) will return 8, which is not expected here.
        if (fromType == PTY_u1) {
            fromSize = 1;
        }
        if (toType == PTY_u1) {
            toSize = 1;
        }
        if (toSize > fromSize) {
            Opcode op = OP_zext;
            if (IsSignedInteger(fromType)) {
                op = OP_sext;
            }
            const MIRIntConst *constVal = safe_cast<MIRIntConst>(cst);
            ASSERT_NOT_NULL(constVal);
            toConst = FoldSignExtendMIRConst(op, toType, static_cast<uint8>(fromSize),
                constVal->GetValue().TruncOrExtend(fromType));
        } else {
            const MIRIntConst *constVal = safe_cast<MIRIntConst>(cst);
            ASSERT_NOT_NULL(constVal);
            MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(toType);
            toConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
                static_cast<uint64>(constVal->GetExtValue()), type);
        }
        return toConst;
    }
    if (IsPrimitiveFloat(fromType) && IsPrimitiveFloat(toType)) {
        MIRConst *toConst = nullptr;
        if (GetPrimTypeBitSize(toType) < GetPrimTypeBitSize(fromType)) {
            DEBUG_ASSERT(GetPrimTypeBitSize(toType) == 32, "We suppot F32 and F64"); // just support 32 or 64
            const MIRDoubleConst *fromValue = safe_cast<MIRDoubleConst>(cst);
            ASSERT_NOT_NULL(fromValue);
            float floatValue = static_cast<float>(fromValue->GetValue());
            MIRFloatConst *toValue = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(floatValue);
            toConst = toValue;
        } else {
            DEBUG_ASSERT(GetPrimTypeBitSize(toType) == 64, "We suppot F32 and F64"); // just support 32 or 64
            const MIRFloatConst *fromValue = safe_cast<MIRFloatConst>(cst);
            ASSERT_NOT_NULL(fromValue);
            double doubleValue = static_cast<double>(fromValue->GetValue());
            MIRDoubleConst *toValue = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(doubleValue);
            toConst = toValue;
        }
        return toConst;
    }
    if (IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) {
        return FoldFloorMIRConst(cst, fromType, toType, false);
    }
    if (IsPrimitiveInteger(fromType) && IsPrimitiveFloat(toType)) {
        return FoldRoundMIRConst(cst, fromType, toType);
    }
    CHECK_FATAL(false, "Unexpected case in ConstFoldTypeCvt");
    return nullptr;
}

ConstvalNode *ConstantFold::FoldTypeCvt(const ConstvalNode &cst, PrimType fromType, PrimType toType) const
{
    MIRConst *toConstValue = FoldTypeCvtMIRConst(*cst.GetConstVal(), fromType, toType);
    if (toConstValue == nullptr) {
        return nullptr;
    }
    ConstvalNode *toConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    toConst->SetPrimType(toConstValue->GetType().GetPrimType());
    toConst->SetConstVal(toConstValue);
    return toConst;
}

// return a primType with bit size >= bitSize (and the nearest one),
// and its signed/float type is the same as ptyp
PrimType GetNearestSizePtyp(uint8 bitSize, PrimType ptyp)
{
    bool isSigned = IsSignedInteger(ptyp);
    bool isFloat = IsPrimitiveFloat(ptyp);
    if (bitSize == 1) { // 1 bit
        return PTY_u1;
    }
    if (bitSize <= 8) { // 8 bit
        return isSigned ? PTY_i8 : PTY_u8;
    }
    if (bitSize <= 16) { // 16 bit
        return isSigned ? PTY_i16 : PTY_u16;
    }
    if (bitSize <= 32) { // 32 bit
        return isFloat ? PTY_f32 : (isSigned ? PTY_i32 : PTY_u32);
    }
    if (bitSize <= 64) { // 64 bit
        return isFloat ? PTY_f64 : (isSigned ? PTY_i64 : PTY_u64);
    }
    return ptyp;
}

size_t GetIntPrimTypeMax(PrimType ptyp)
{
    switch (ptyp) {
        case PTY_u1:
            return 1;
        case PTY_u8:
            return UINT8_MAX;
        case PTY_i8:
            return INT8_MAX;
        case PTY_u16:
            return UINT16_MAX;
        case PTY_i16:
            return INT16_MAX;
        case PTY_u32:
            return UINT32_MAX;
        case PTY_i32:
            return INT32_MAX;
        case PTY_u64:
            return UINT64_MAX;
        case PTY_i64:
            return INT64_MAX;
        default:
            CHECK_FATAL(false, "NYI");
    }
}

ssize_t GetIntPrimTypeMin(PrimType ptyp)
{
    if (IsUnsignedInteger(ptyp)) {
        return 0;
    }
    switch (ptyp) {
        case PTY_i8:
            return INT8_MIN;
        case PTY_i16:
            return INT16_MIN;
        case PTY_i32:
            return INT32_MIN;
        case PTY_i64:
            return INT64_MIN;
        default:
            CHECK_FATAL(false, "NYI");
    }
}

static bool IsCvtEliminatable(PrimType fromPtyp, PrimType destPtyp, Opcode op, Opcode opndOp)
{
    if (op != OP_cvt || (opndOp == OP_zext || opndOp == OP_sext)) {
        return false;
    }
    if (GetPrimTypeSize(fromPtyp) != GetPrimTypeSize(destPtyp)) {
        return false;
    }
    return (IsPossible64BitAddress(fromPtyp) && IsPossible64BitAddress(destPtyp)) ||
        (IsPossible32BitAddress(fromPtyp) && IsPossible32BitAddress(destPtyp)) ||
        (IsPrimitivePureScalar(fromPtyp) && IsPrimitivePureScalar(destPtyp));
}

std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::FoldTypeCvt(TypeCvtNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    if (GetPrimTypeSize(node->GetPrimType()) > k8ByteSize) {
        return {node, std::nullopt};
    }
    std::pair<BaseNode*, std::optional<IntVal>> p = DispatchFold(node->Opnd(0));
    ConstvalNode *cst = safe_cast<ConstvalNode>(p.first);
    PrimType destPtyp = node->GetPrimType();
    PrimType fromPtyp = node->FromType();
    if (cst != nullptr) {
        switch (node->GetOpCode()) {
            case OP_ceil: {
                result = FoldCeil(*cst, fromPtyp, destPtyp);
                break;
            }
            case OP_cvt: {
                result = FoldTypeCvt(*cst, fromPtyp, destPtyp);
                break;
            }
            case OP_floor: {
                result = FoldFloor(*cst, fromPtyp, destPtyp);
                break;
            }
            case OP_trunc: {
                result = FoldTrunc(*cst, fromPtyp, destPtyp);
                break;
            }
            default:
                DEBUG_ASSERT(false, "Unexpected opcode in TypeCvtNodeConstFold");
                break;
        }
    } else if (IsCvtEliminatable(fromPtyp, destPtyp, node->GetOpCode(), p.first->GetOpCode())) {
        // the cvt is redundant
        return std::make_pair(p.first, p.second ? IntVal(*p.second, node->GetPrimType()) : p.second);
    }
    if (result == nullptr) {
        BaseNode *e = PairToExpr(node->Opnd(0)->GetPrimType(), p);
        if (e != node->Opnd(0)) {
            result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(
                Opcode(node->GetOpCode()), PrimType(node->GetPrimType()), PrimType(node->FromType()), e);
        } else {
            result = node;
        }
    }
    return std::make_pair(result, std::nullopt);
}

MIRConst *ConstantFold::FoldSignExtendMIRConst(Opcode opcode, PrimType resultType, uint8 size, const IntVal &val) const
{
    uint64 result = opcode == OP_sext ? static_cast<uint64>(val.GetSXTValue(size)) : val.GetZXTValue(size);
    MIRType &type = *GlobalTables::GetTypeTable().GetPrimType(resultType);
    MIRIntConst *constValue = GlobalTables::GetIntConstTable().GetOrCreateIntConst(result, type);
    return constValue;
}

ConstvalNode *ConstantFold::FoldSignExtend(Opcode opcode, PrimType resultType, uint8 size,
                                           const ConstvalNode &cst) const
{
    ConstvalNode *resultConst = mirModule->CurFuncCodeMemPool()->New<ConstvalNode>();
    const auto *intCst = safe_cast<MIRIntConst>(cst.GetConstVal());
    ASSERT_NOT_NULL(intCst);
    IntVal val = intCst->GetValue().TruncOrExtend(size, opcode == OP_sext);
    MIRConst *toConst = FoldSignExtendMIRConst(opcode, resultType, size, val);
    resultConst->SetPrimType(toConst->GetType().GetPrimType());
    resultConst->SetConstVal(toConst);
    return resultConst;
}

// check if truncation is redundant due to dread or iread having same effect
static bool ExtractbitsRedundant(const ExtractbitsNode &x, MIRFunction &f)
{
    if (GetPrimTypeSize(x.GetPrimType()) == k8ByteSize) {
        return false;  // this is trying to be conservative
    }
    BaseNode *opnd = x.Opnd(0);
    MIRType *mirType = nullptr;
    if (opnd->GetOpCode() == OP_dread) {
        DreadNode *dread = static_cast<DreadNode*>(opnd);
        MIRSymbol *sym = f.GetLocalOrGlobalSymbol(dread->GetStIdx());
        ASSERT_NOT_NULL(sym);
        mirType = sym->GetType();
    } else if (opnd->GetOpCode() == OP_iread) {
        IreadNode *iread = static_cast<IreadNode*>(opnd);
        MIRPtrType *ptrType =
            dynamic_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx()));
        if (ptrType == nullptr) {
            return false;
        }
        mirType = ptrType->GetPointedType();
    } else if (opnd->GetOpCode() == OP_extractbits &&
                x.GetBitsSize() > static_cast<ExtractbitsNode*>(opnd)->GetBitsSize()) {
        return (x.GetOpCode() == OP_zext && x.GetPrimType() == opnd->GetPrimType() &&
            IsUnsignedInteger(opnd->GetPrimType()));
    } else {
        return false;
    }
    return IsPrimitiveInteger(mirType->GetPrimType()) &&
            ((x.GetOpCode() == OP_zext && IsUnsignedInteger(opnd->GetPrimType())) ||
            (x.GetOpCode() == OP_sext && IsSignedInteger(opnd->GetPrimType()))) &&
            mirType->GetSize() * kBitSizePerByte == x.GetBitsSize() &&
            mirType->GetPrimType() == x.GetPrimType();
}

// sext and zext also handled automatically
std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::FoldExtractbits(ExtractbitsNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    uint8 offset = node->GetBitsOffset();
    uint8 size = node->GetBitsSize();
    Opcode opcode = node->GetOpCode();
    std::pair<BaseNode*, std::optional<IntVal>> p = DispatchFold(node->Opnd(0));
    ConstvalNode *cst = safe_cast<ConstvalNode>(p.first);
    if (cst != nullptr && (opcode == OP_sext || opcode == OP_zext)) {
        result = FoldSignExtend(opcode, node->GetPrimType(), size, *cst);
        return std::make_pair(result, std::nullopt);
    }
    BaseNode *e = PairToExpr(node->Opnd(0)->GetPrimType(), p);
    if (e != node->Opnd(0)) {
        result = mirModule->CurFuncCodeMemPool()->New<ExtractbitsNode>(opcode, PrimType(node->GetPrimType()), offset,
                                                                       size, e);
    } else {
        result = node;
    }
    // check for consecutive and redundant extraction of same bits
    BaseNode *opnd = result->Opnd(0);
    DEBUG_ASSERT(opnd != nullptr, "opnd shoule not be null");
    Opcode opndOp = opnd->GetOpCode();
    if (opndOp == OP_extractbits || opndOp == OP_sext || opndOp == OP_zext) {
        uint8 opndOffset = static_cast<ExtractbitsNode*>(opnd)->GetBitsOffset();
        uint8 opndSize = static_cast<ExtractbitsNode*>(opnd)->GetBitsSize();
        if (offset == opndOffset && size == opndSize) {
            result->SetOpnd(opnd->Opnd(0), 0);  // delete the redundant extraction
        }
    }
    if (offset == 0 && size >= k8ByteSize && IsPowerOf2(size)) {
        if (ExtractbitsRedundant(*static_cast<ExtractbitsNode*>(result), *mirModule->CurFunction())) {
            return std::make_pair(result->Opnd(0), std::nullopt);
        }
    }
    return std::make_pair(result, std::nullopt);
}

std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::FoldIread(IreadNode *node)
{
    CHECK_NULL_FATAL(node);
    std::pair<BaseNode*, std::optional<IntVal>> p = DispatchFold(node->Opnd(0));
    BaseNode *e = PairToExpr(node->Opnd(0)->GetPrimType(), p);
    node->SetOpnd(e, 0);
    BaseNode *result = node;
    if (e->GetOpCode() != OP_addrof) {
        return std::make_pair(result, std::nullopt);
    }

    AddrofNode *addrofNode = static_cast<AddrofNode*>(e);
    MIRSymbol *msy = mirModule->CurFunction()->GetLocalOrGlobalSymbol(addrofNode->GetStIdx());
    DEBUG_ASSERT(msy != nullptr, "nullptr check");
    TyIdx typeId = msy->GetTyIdx();
    CHECK_FATAL(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
    MIRType *msyType = GlobalTables::GetTypeTable().GetTypeTable()[typeId];
    MIRPtrType *ptrType = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(node->GetTyIdx()));
    // If the high level type of iaddrof/iread doesn't match
    // the type of addrof's rhs, this optimization cannot be done.
    if (ptrType->GetPointedType() != msyType) {
        return std::make_pair(result, std::nullopt);
    }

    Opcode op = node->GetOpCode();
    if (op == OP_iread) {
        result = mirModule->CurFuncCodeMemPool()->New<AddrofNode>(OP_dread, node->GetPrimType(), addrofNode->GetStIdx(),
                                                                  node->GetFieldID() + addrofNode->GetFieldID());
    }
    return std::make_pair(result, std::nullopt);
}

bool ConstantFold::IntegerOpIsOverflow(Opcode op, PrimType primType, int64 cstA, int64 cstB)
{
    switch (op) {
        case OP_add: {
            int64 res = static_cast<int64>(static_cast<uint64>(cstA) + static_cast<uint64>(cstB));
            if (IsUnsignedInteger(primType)) {
                return static_cast<uint64>(res) < static_cast<uint64>(cstA);
            }
            auto rightShiftNumToGetSignFlag = GetPrimTypeBitSize(primType) - 1;
            return (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
                    static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag) &&
                   (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
                    static_cast<uint64>(cstB) >> rightShiftNumToGetSignFlag);
        }
        case OP_sub: {
            if (IsUnsignedInteger(primType)) {
                return cstA < cstB;
            }
            int64 res = static_cast<int64>(static_cast<uint64>(cstA) - static_cast<uint64>(cstB));
            auto rightShiftNumToGetSignFlag = GetPrimTypeBitSize(primType) - 1;
            return (static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag !=
                    static_cast<uint64>(cstB) >> rightShiftNumToGetSignFlag) &&
                   (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
                    static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag);
        }
        default: {
            return false;
        }
    }
}

std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::FoldBinary(BinaryNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    std::optional<IntVal> sum = std::nullopt;
    Opcode op = node->GetOpCode();
    PrimType primType = node->GetPrimType();
    PrimType lPrimTypes = node->Opnd(0)->GetPrimType();
    PrimType rPrimTypes = node->Opnd(1)->GetPrimType();
    std::pair<BaseNode*, std::optional<IntVal>> lp = DispatchFold(node->Opnd(0));
    std::pair<BaseNode*, std::optional<IntVal>> rp = DispatchFold(node->Opnd(1));
    BaseNode *l = lp.first;
    BaseNode *r = rp.first;
    ASSERT_NOT_NULL(r);
    ConstvalNode *lConst = safe_cast<ConstvalNode>(l);
    ConstvalNode *rConst = safe_cast<ConstvalNode>(r);
    bool isInt = IsPrimitiveInteger(primType);

    if (lConst != nullptr && rConst != nullptr) {
        MIRConst *lConstVal = lConst->GetConstVal();
        MIRConst *rConstVal = rConst->GetConstVal();
        ASSERT_NOT_NULL(lConstVal);
        ASSERT_NOT_NULL(rConstVal);
        // Don't fold div by 0, for floats div by 0 is well defined.
        if ((op == OP_div || op == OP_rem) && isInt &&
            !IsDivSafe(static_cast<MIRIntConst &>(*lConstVal), static_cast<MIRIntConst &>(*rConstVal), primType)) {
            result = NewBinaryNode(node, op, primType, lConst, rConst);
        } else {
            // 4 + 2 -> return a pair(result = ConstValNode(6), sum = 0)
            // Create a new ConstvalNode for 6 but keep the sum = 0. This simplify the
            // logic since the alternative is to return pair(result = nullptr, sum = 6).
            // Doing so would introduce many nullptr checks in the code. See previous
            // commits that implemented that logic for a comparison.
            result = FoldConstBinary(op, primType, *lConst, *rConst);
        }
    } else if (lConst != nullptr && isInt) {
        MIRIntConst *mcst = safe_cast<MIRIntConst>(lConst->GetConstVal());
        ASSERT_NOT_NULL(mcst);
        PrimType cstTyp = mcst->GetType().GetPrimType();
        IntVal cst = mcst->GetValue();
        if (op == OP_add) {
            if (IsSignedInteger(cstTyp) && rp.second &&
                IntegerOpIsOverflow(OP_add, cstTyp, cst.GetExtValue(), rp.second->GetExtValue())) {
                // do not introduce signed integer overflow
                result = NewBinaryNode(node, op, primType, l, PairToExpr(rPrimTypes, rp));
            } else {
                sum = cst + rp.second;
                result = r;
            }
        } else if (op == OP_sub && r->GetPrimType() != PTY_u1) {
            // We exclude u1 type for fixing the following wrong example:
            // before cf:
            //   sub i32 (constval i32 17, eq u1 i32 (dread i32 %i, constval i32 16)))
            // after cf:
            //   add i32 (cvt i32 u1 (neg u1 (eq u1 i32 (dread i32 %i, constval i32 16))), constval i32 17))
            sum = cst - rp.second;
            if (GetPrimTypeSize(r->GetPrimType()) < GetPrimTypeSize(primType)) {
                r = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, r->GetPrimType(), r);
            }
            result = NegateTree(r);
        } else if ((op == OP_mul || op == OP_div || op == OP_rem || op == OP_ashr || op == OP_lshr || op == OP_shl ||
                    op == OP_band) &&
                    cst == 0) {
            // 0 * X -> 0
            // 0 / X -> 0
            // 0 % X -> 0
            // 0 >> X -> 0
            // 0 << X -> 0
            // 0 & X -> 0
            // 0 && X -> 0
            result = mirModule->GetMIRBuilder()->CreateIntConst(0, cstTyp);
        } else if (op == OP_mul && cst == 1) {
            // 1 * X --> X
            sum = rp.second;
            result = r;
        } else if (op == OP_bior && cst == -1) {
            // (-1) | X -> -1
            result = mirModule->GetMIRBuilder()->CreateIntConst(static_cast<uint64>(-1), cstTyp);
        } else if (op == OP_mul && rp.second.has_value() && *rp.second != 0) {
            // lConst * (X + konst) -> the pair [(lConst*X), (lConst*konst)]
            sum = cst * rp.second;
            if (GetPrimTypeSize(primType) > GetPrimTypeSize(rp.first->GetPrimType())) {
                rp.first = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, PTY_i32, rp.first);
            }
            result = NewBinaryNode(node, OP_mul, primType, lConst, rp.first);
        } else if ((op == OP_bior || op == OP_bxor) && cst == 0) {
            // 0 | X -> X
            // 0 ^ X -> X
            sum = rp.second;
            result = r;
        } else {
            result = NewBinaryNode(node, op, primType, l, PairToExpr(rPrimTypes, rp));
        }
        if (!IsNoCvtNeeded(result->GetPrimType(), primType)) {
            result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, result->GetPrimType(), result);
        }
    } else if (rConst != nullptr && isInt) {
        MIRIntConst *mcst = safe_cast<MIRIntConst>(rConst->GetConstVal());
        ASSERT_NOT_NULL(mcst);
        PrimType cstTyp = mcst->GetType().GetPrimType();
        IntVal cst = mcst->GetValue();
        if (op == OP_add) {
            if (lp.second && IntegerOpIsOverflow(op, cstTyp, lp.second->GetExtValue(), cst.GetExtValue())) {
                result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), PairToExpr(rPrimTypes, rp));
            } else {
                result = l;
                sum = lp.second + cst;
            }
        } else if (op == OP_sub && (!cst.IsSigned() || !cst.IsMinValue())) {
            result = l;
            sum = lp.second - cst;
        } else if ((op == OP_mul || op == OP_band) && cst == 0) {
            // X * 0 -> 0
            // X & 0 -> 0
            // X && 0 -> 0
            result = mirModule->GetMIRBuilder()->CreateIntConst(0, cstTyp);
        } else if ((op == OP_mul || op == OP_div) && cst == 1) {
            // case [X * 1 -> X]
            // case [X / 1 = X]
            sum = lp.second;
            result = l;
        } else if (op == OP_div && !lp.second.has_value() && l->GetOpCode() == OP_mul &&
                IsSignedInteger(primType) && IsSignedInteger(lPrimTypes) && IsSignedInteger(rPrimTypes)) {
            // temporary fix for constfold of mul/div in DejaGnu
            // Later we need a more formal interface for pattern match
            // X * Y / Y -> X
            BaseNode *x = l->Opnd(0);
            BaseNode *y = l->Opnd(1);
            ConstvalNode *xConst = safe_cast<ConstvalNode>(x);
            ConstvalNode *yConst = safe_cast<ConstvalNode>(y);
            bool foldMulDiv = false;
            if (yConst != nullptr && xConst == nullptr &&
                IsSignedInteger(x->GetPrimType()) && IsSignedInteger(y->GetPrimType())) {
                MIRIntConst *yCst = safe_cast<MIRIntConst>(yConst->GetConstVal());
                ASSERT_NOT_NULL(yCst);
                IntVal mulCst = yCst->GetValue();
                if (mulCst.GetBitWidth() == cst.GetBitWidth() && mulCst.IsSigned() == cst.IsSigned() &&
                    mulCst.GetExtValue() == cst.GetExtValue()) {
                    foldMulDiv = true;
                    result = x;
                }
            } else if (xConst != nullptr && yConst == nullptr &&
                        IsSignedInteger(x->GetPrimType()) && IsSignedInteger(y->GetPrimType())) {
                MIRIntConst *xCst = safe_cast<MIRIntConst>(xConst->GetConstVal());
                ASSERT_NOT_NULL(xCst);
                IntVal mulCst = xCst->GetValue();
                if (mulCst.GetBitWidth() == cst.GetBitWidth() && mulCst.IsSigned() == cst.IsSigned() &&
                    mulCst.GetExtValue() == cst.GetExtValue()) {
                    foldMulDiv = true;
                    result = y;
                }
            }
            if (!foldMulDiv) {
                result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), r);
            }
        } else if (op == OP_mul && lp.second.has_value() && *lp.second != 0 && lp.second->GetSXTValue() > -kMaxOffset) {
            // (X + konst) * rConst -> the pair [(X*rConst), (konst*rConst)]
            sum = lp.second * cst;
            if (GetPrimTypeSize(primType) > GetPrimTypeSize(lp.first->GetPrimType())) {
                lp.first = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, PTY_i32, lp.first);
            }
            if (lp.first->GetOpCode() == OP_neg && cst == -1) {
                // special case: ((-X) + konst) * (-1) -> the pair [(X), -konst]
                result = lp.first->Opnd(0);
            } else {
                result = NewBinaryNode(node, OP_mul, primType, lp.first, rConst);
            }
        } else if (op == OP_band && cst == -1) {
            // X & (-1) -> X
            sum = lp.second;
            result = l;
        } else if (op == OP_band && ContiguousBitsOf1(cst.GetZXTValue()) &&
                   (!lp.second.has_value() || lp.second == 0)) {
            bool fold2extractbits = false;
            if (l->GetOpCode() == OP_ashr || l->GetOpCode() == OP_lshr) {
                BinaryNode *shrNode = static_cast<BinaryNode *>(l);
                if (shrNode->Opnd(1)->GetOpCode() == OP_constval) {
                    ConstvalNode *shrOpnd = static_cast<ConstvalNode *>(shrNode->Opnd(1));
                    int64 shrAmt = static_cast<MIRIntConst*>(shrOpnd->GetConstVal())->GetExtValue();
                    uint64 ucst = cst.GetZXTValue();
                    uint32 bsize = 0;
                    do {
                        bsize++;
                        ucst >>= 1;
                    } while (ucst != 0);
                    if (shrAmt + static_cast<int64>(bsize) <=
                        static_cast<int64>(GetPrimTypeSize(primType) * kBitSizePerByte) &&
                        static_cast<uint64>(shrAmt) < GetPrimTypeSize(primType) * kBitSizePerByte) {
                        fold2extractbits = true;
                        // change to use extractbits
                        result = mirModule->GetMIRBuilder()->CreateExprExtractbits(OP_extractbits,
                            GetUnsignedPrimType(primType), static_cast<uint32>(shrAmt), bsize, shrNode->Opnd(0));
                        sum = std::nullopt;
                    }
                }
            }
            if (!fold2extractbits) {
                result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), r);
                sum = std::nullopt;
            }
        } else if (op == OP_bior && cst == -1) {
            // X | (-1) -> -1
            result = mirModule->GetMIRBuilder()->CreateIntConst(-1ULL, cstTyp);
        } else if ((op == OP_ashr || op == OP_lshr || op == OP_shl || op == OP_bior || op == OP_bxor) && cst == 0) {
            // X >> 0 -> X
            // X << 0 -> X
            // X | 0 -> X
            // X ^ 0 -> X
            sum = lp.second;
            result = l;
        } else if (op == OP_bxor && cst == 1 && primType != PTY_u1) {
            // bxor i32 (
            //   cvt i32 u1 (regread u1 %13),
            //  constValue i32 1),
            result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), PairToExpr(rPrimTypes, rp));
            if (l->GetOpCode() == OP_cvt && (!lp.second || lp.second == 0)) {
                TypeCvtNode *cvtNode = static_cast<TypeCvtNode*>(l);
                if (cvtNode->Opnd(0)->GetPrimType() == PTY_u1) {
                    BaseNode *base = cvtNode->Opnd(0);
                    BaseNode *constValue = mirModule->GetMIRBuilder()->CreateIntConst(1, base->GetPrimType());
                    std::pair<BaseNode*, std::optional<IntVal>> p = DispatchFold(base);
                    BinaryNode *temp = NewBinaryNode(node, op, PTY_u1, PairToExpr(base->GetPrimType(), p), constValue);
                    result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, PTY_u1, temp);
                }
            }
        } else if (op == OP_rem && cst == 1) {
            // X % 1 -> 0
            result = mirModule->GetMIRBuilder()->CreateIntConst(0, cstTyp);
        } else {
            result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), r);
        }
        if (!IsNoCvtNeeded(result->GetPrimType(), primType)) {
            result = mirModule->CurFuncCodeMemPool()->New<TypeCvtNode>(OP_cvt, primType, result->GetPrimType(), result);
        }
    } else if (isInt && (op == OP_add || op == OP_sub)) {
        if (op == OP_add) {
            result = NewBinaryNode(node, op, primType, l, r);
            sum = lp.second + rp.second;
        } else if (r != nullptr && node->Opnd(1)->GetOpCode() == OP_sub && r->GetOpCode() == OP_neg) {
            // if fold is (x - (y - z))    ->     (x - neg(z)) - y
            // (x - neg(z)) Could cross the int limit
            // return node
            result = node;
        } else {
            result = NewBinaryNode(node, op, primType, l, r);
            sum = lp.second - rp.second;
        }
    } else {
        result = NewBinaryNode(node, op, primType, PairToExpr(lPrimTypes, lp), PairToExpr(rPrimTypes, rp));
    }
    return std::make_pair(result, sum);
}

BaseNode *ConstantFold::SimplifyDoubleConstvalCompare(CompareNode &node, bool isRConstval, bool isGtOrLt) const
{
    if (isRConstval) {
        ConstvalNode *constNode = static_cast<ConstvalNode*>(node.Opnd(1));
        if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
            const CompareNode *compNode = static_cast<CompareNode*>(node.Opnd(0));
            return mirModule->CurFuncCodeMemPool()->New<CompareNode>(node.GetOpCode(),
                node.GetPrimType(), compNode->GetOpndType(), compNode->Opnd(0), compNode->Opnd(1));
        }
    } else {
        ConstvalNode *constNode = static_cast<ConstvalNode*>(node.Opnd(0));
        if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
            const CompareNode *compNode = static_cast<CompareNode*>(node.Opnd(1));
            if (isGtOrLt) {
                return mirModule->CurFuncCodeMemPool()->New<CompareNode>(node.GetOpCode(),
                    node.GetPrimType(), compNode->GetOpndType(), compNode->Opnd(1), compNode->Opnd(0));
            } else {
                return mirModule->CurFuncCodeMemPool()->New<CompareNode>(node.GetOpCode(),
                    node.GetPrimType(), compNode->GetOpndType(), compNode->Opnd(0), compNode->Opnd(1));
            }
        }
    }
    return &node;
}

BaseNode *ConstantFold::SimplifyDoubleCompare(CompareNode &compareNode) const
{
    // See arm manual B.cond(P2993) and FCMP(P1091)
    CompareNode *node = &compareNode;
    BaseNode *result = node;
    BaseNode *l = node->Opnd(0);
    BaseNode *r = node->Opnd(1);
    if (node->GetOpCode() == OP_ne || node->GetOpCode() == OP_eq) {
        if ((l->GetOpCode() == OP_cmp && r->GetOpCode() == OP_constval) ||
            (r->GetOpCode() == OP_cmp && l->GetOpCode() == OP_constval)) {
            result = SimplifyDoubleConstvalCompare(*node, (l->GetOpCode() == OP_cmp && r->GetOpCode() == OP_constval));
        } else if (node->GetOpCode() == OP_ne && r->GetOpCode() == OP_constval) {
            // ne (u1 x, constValue 0)  <==> x
            ConstvalNode *constNode = static_cast<ConstvalNode*>(r);
            if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
                BaseNode *opnd = l;
                do {
                    if (opnd->GetPrimType() == PTY_u1 || (l->GetOpCode() == OP_ne || l->GetOpCode() == OP_eq)) {
                        result = opnd;
                        break;
                    } else if (opnd->GetOpCode() == OP_cvt) {
                        TypeCvtNode *cvtNode = static_cast<TypeCvtNode*>(opnd);
                        opnd = cvtNode->Opnd(0);
                    } else {
                        opnd = nullptr;
                    }
                } while (opnd != nullptr);
            }
        } else if (node->GetOpCode() == OP_eq && r->GetOpCode() == OP_constval) {
            ConstvalNode *constNode = static_cast<ConstvalNode*>(r);
            if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero() &&
                (l->GetOpCode() == OP_ne || l->GetOpCode() == OP_eq)) {
                auto resOp = l->GetOpCode() == OP_ne ? OP_eq : OP_ne;
                result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(
                    resOp, l->GetPrimType(), static_cast<CompareNode*>(l)->GetOpndType(), l->Opnd(0), l->Opnd(1));
            }
        }
    } else if (node->GetOpCode() == OP_gt || node->GetOpCode() == OP_lt) {
        if ((l->GetOpCode() == OP_cmp && r->GetOpCode() == OP_constval) ||
            (r->GetOpCode() == OP_cmp && l->GetOpCode() == OP_constval)) {
            result = SimplifyDoubleConstvalCompare(*node,
                (l->GetOpCode() == OP_cmp && r->GetOpCode() == OP_constval), true);
        }
    }
    return result;
}

std::pair<BaseNode*, std::optional<IntVal>> ConstantFold::FoldCompare(CompareNode *node)
{
    CHECK_NULL_FATAL(node);
    BaseNode *result = nullptr;
    std::pair<BaseNode*, std::optional<IntVal>> lp = DispatchFold(node->Opnd(0));
    std::pair<BaseNode*, std::optional<IntVal>> rp = DispatchFold(node->Opnd(1));
    ConstvalNode *lConst = safe_cast<ConstvalNode>(lp.first);
    ConstvalNode *rConst = safe_cast<ConstvalNode>(rp.first);
    Opcode opcode = node->GetOpCode();
    if (lConst != nullptr && rConst != nullptr) {
        result = FoldConstComparison(node->GetOpCode(), node->GetPrimType(), node->GetOpndType(), *lConst, *rConst);
    } else if (lConst != nullptr && rConst == nullptr && opcode != OP_cmp &&
               lConst->GetConstVal()->GetKind() == kConstInt) {
        BaseNode *l = lp.first;
        BaseNode *r = PairToExpr(node->Opnd(1)->GetPrimType(), rp);
        result = FoldConstComparisonReverse(opcode, node->GetPrimType(), node->GetOpndType(), *l, *r);
    } else {
        BaseNode *l = PairToExpr(node->Opnd(0)->GetPrimType(), lp);
        BaseNode *r = PairToExpr(node->Opnd(1)->GetPrimType(), rp);
        if (l != node->Opnd(0) || r != node->Opnd(1)) {
            result = mirModule->CurFuncCodeMemPool()->New<CompareNode>(
                Opcode(node->GetOpCode()), PrimType(node->GetPrimType()), PrimType(node->GetOpndType()), l, r);
        } else {
            result = node;
        }
        auto *compareNode = static_cast<CompareNode*>(result);
        CHECK_NULL_FATAL(compareNode);
        result = SimplifyDoubleCompare(*compareNode);
    }
    return std::make_pair(result, std::nullopt);
}

BaseNode *ConstantFold::Fold(BaseNode *node)
{
    if (node == nullptr || kOpcodeInfo.IsStmt(node->GetOpCode())) {
        return nullptr;
    }
    std::pair<BaseNode*, std::optional<IntVal>> p = DispatchFold(node);
    BaseNode *result = PairToExpr(node->GetPrimType(), p);
    if (result == node) {
        result = nullptr;
    }
    return result;
}

}  // namespace maple
