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

#include "ecmascript/compiler/array_bounds_check_elimination.h"

namespace panda::ecmascript::kungfu {
void ArrayBoundsCheckElimination::Run()
{
    bounds_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    indexCheckInfo_.resize(circuit_->GetMaxGateId() + 1, nullptr);
    graphLinearizer_.SetScheduleJSOpcode();
    graphLinearizer_.LinearizeGraph();
    CalcBounds(graphLinearizer_.GetEntryRegion(), nullptr);
}

/*
    i_lower + c_lower <= x <= i_upper + c_upper
    Initially, when nothing about the bounds is known yet, every instrution has the bounds:
        MIN <= x <= MAX
*/
ArrayBoundsCheckElimination::Bound::Bound()
{
    lower_ = INT_MIN;
    upper_ = INT_MAX;
    lowerGate_ = Circuit::NullGate();
    upperGate_ = Circuit::NullGate();
}

ArrayBoundsCheckElimination::Bound::Bound(int lower, GateRef lowerGate, int upper, GateRef upperGate)
{
    lower_ = lower;
    upper_ = upper;
    lowerGate_ = lowerGate;
    upperGate_ = upperGate;
}

ArrayBoundsCheckElimination::Bound::Bound(TypedBinOp op, GateRef gate, int constant)
{
    switch (op) {
        case TypedBinOp::TYPED_EQ: {
            lower_ = constant;
            lowerGate_ = gate;
            upper_ = constant;
            upperGate_ = gate;
            break;
        }
        case TypedBinOp::TYPED_NOTEQ: {
            lower_ = INT_MIN;
            lowerGate_ = Circuit::NullGate();
            upper_ = INT_MAX;
            upperGate_ = Circuit::NullGate();
            if (gate == Circuit::NullGate()) {
                if (constant == INT_MIN) {
                    lower_++;
                }
                if (constant == INT_MAX) {
                    upper_--;
                }
            }
            break;
        }
        case TypedBinOp::TYPED_GREATEREQ: {
            lower_ = constant;
            lowerGate_ = gate;
            upper_ = INT_MAX;
            upperGate_ = Circuit::NullGate();
            break;
        }
        case TypedBinOp::TYPED_LESSEQ: {
            lower_ = INT_MIN;
            lowerGate_ = Circuit::NullGate();
            upper_ = constant;
            upperGate_ = gate;
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "unknown binary op";
            UNREACHABLE();
        }
    }
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::AndOp(Bound *bound, Bound *b)
{
    // Update lower bound
    if (bound->lowerGate_ == b->lowerGate_) {
        bound->lower_ = std::max(bound->lower_, b->lower_);
    }
    if (b->HasLower()) {
        bool set = true;
        if (bound->lowerGate_ != Circuit::NullGate() && b->lowerGate_ != Circuit::NullGate()) {
            auto boundLowerGateRegion = graphLinearizer_.GateToRegion(bound->lowerGate_);
            auto bLowerGateRegion = graphLinearizer_.GateToRegion(b->lowerGate_);
            int32_t boundLowerDominatorDepth = -1;
            if (boundLowerGateRegion) {
                boundLowerDominatorDepth = boundLowerGateRegion->GetDepth();
            }
            int32_t bLowerDominatorDepth = -1;
            if (bLowerGateRegion) {
                bLowerDominatorDepth = bLowerGateRegion->GetDepth();
            }
            set = (boundLowerDominatorDepth < bLowerDominatorDepth);
        }
        if (set) {
            bound->lower_ = b->lower_;
            bound->lowerGate_ = b->lowerGate_;
        }
    }

    // Update upper bound
    if (bound->upperGate_ == b->upperGate_) {
        bound->upper_ = std::min(bound->upper_, b->upper_);
    }
    if (b->HasUpper()) {
        bool set = true;
        if (bound->upperGate_ != Circuit::NullGate() && b->upperGate_ != Circuit::NullGate()) {
            auto boundUpperGateRegion = graphLinearizer_.GateToRegion(bound->upperGate_);
            auto bUpperGateRegion = graphLinearizer_.GateToRegion(b->upperGate_);
            int32_t boundUpperDominatorDepth = -1;
            if (boundUpperGateRegion) {
                boundUpperDominatorDepth = boundUpperGateRegion->GetDepth();
            }
            int32_t bUpperDominatorDepth = -1;
            if (bUpperGateRegion) {
                bUpperDominatorDepth = bUpperGateRegion->GetDepth();
            }
            set = (boundUpperDominatorDepth < bUpperDominatorDepth);
        }
        if (set) {
            bound->upper_ = b->upper_;
            bound->upperGate_ = b->upperGate_;
        }
    }

    return bound;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::OrOp(Bound *bound, Bound *b)
{
    // Update lower bound
    if (bound->lowerGate_ != b->lowerGate_) {
        bound->lowerGate_ = Circuit::NullGate();
        bound->lower_ = INT_MIN;
    } else {
        bound->lower_ = std::min(bound->lower_, b->lower_);
    }
    // Update upper bound
    if (bound->upperGate_ != b->upperGate_) {
        bound->upperGate_ = Circuit::NullGate();
        bound->upper_ = INT_MAX;
    } else {
        bound->upper_ = std::max(bound->upper_, b->upper_);
    }

    return bound;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::DoConstant(GateRef gate)
{
    int constValue = static_cast<int>(acc_.GetConstantValue(gate));
    return chunk_->New<Bound>(constValue, Circuit::NullGate(), constValue, Circuit::NullGate());
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::DoBinaryArithmeticOp(GateRef gate)
{
    auto op = acc_.GetTypedBinaryOp(gate);
    auto x = FindBoundGate(acc_.GetValueIn(gate, 0));
    auto y = FindBoundGate(acc_.GetValueIn(gate, 1));
    if (!acc_.IsConstant(x) || !acc_.IsConstant(y)) { // One of the operands must be non-constant!
        if (op == TypedBinOp::TYPED_AND && (acc_.IsConstant(x) || acc_.IsConstant(y))) {
            int constValue = 0;
            if (acc_.IsConstant(x)) {
                constValue = static_cast<int>(acc_.GetConstantValue(x));
            } else {
                constValue = static_cast<int>(acc_.GetConstantValue(y));
            }
            if (constValue >= 0) {
                return chunk_->New<Bound>(0, Circuit::NullGate(), constValue, Circuit::NullGate());
            }
        } else if (op == TypedBinOp::TYPED_MOD) {
            Bound *xBound = GetBound(x);
            if (xBound->Lower() >= 0 && xBound->LowerGate() == Circuit::NullGate() && IsArrayLength(y)) {
                return chunk_->New<Bound>(0, Circuit::NullGate(), -1, y);
            } else if (xBound->HasLower() && xBound->Lower() >= 0 && acc_.IsConstant(y)
                        && acc_.GetConstantValue(y) != 0) {
                int constValue = static_cast<int>(acc_.GetConstantValue(y));
                if (constValue != INT_MIN) {
                    return chunk_->New<Bound>(0, Circuit::NullGate(), abs(constValue) - 1, Circuit::NullGate());
                } else {
                    return chunk_->New<Bound>();
                }
            } else {
                return chunk_->New<Bound>();
            }
        } else if (((acc_.IsConstant(x) || acc_.IsConstant(y)) && op == TypedBinOp::TYPED_ADD) ||
            (acc_.IsConstant(y) && op == TypedBinOp::TYPED_SUB)) {
            // x is constant, y is variable.
            if (acc_.IsConstant(y)) {
                std::swap(x, y);
            }

            // Add, Constant now in x
            int constValue = static_cast<int>(acc_.GetConstantValue(x));
            if (op == TypedBinOp::TYPED_SUB) {
                constValue = -constValue;
            }

            Bound *bound = GetBound(y);
            if (bound == nullptr) {
                LOG_ECMA(FATAL) << "ArrayBoundsCheckElimination::DoBinaryArithmeticOp:bound is nullptr";
                UNREACHABLE();
            }
            if (!bound->HasUpper() || !bound->HasLower()) {
                return chunk_->New<Bound>();
            }

            int lower = bound->Lower();
            int upper = bound->Upper();
            int newLower = lower + constValue;
            int newUpper = upper + constValue;
            bool overflow = ((constValue < 0 && (newLower > lower)) ||
                                (constValue > 0 && (newUpper < upper)));
            if (overflow) {
                return chunk_->New<Bound>();
            } else {
                return chunk_->New<Bound>(newLower, bound->LowerGate(), newUpper, bound->UpperGate());
            }
        } else if (op == TypedBinOp::TYPED_SUB) {
            Bound *bound = GetBound(x);
            if (bound == nullptr) {
                LOG_ECMA(FATAL) << "ArrayBoundsCheckElimination::DoBinaryArithmeticOp:bound is nullptr";
                UNREACHABLE();
            }
            if (bound->LowerGate() == y) {
                return chunk_->New<Bound>(TypedBinOp::TYPED_GREATEREQ, Circuit::NullGate(), bound->Lower());
            } else {
                return chunk_->New<Bound>();
            }
        } else {
            return chunk_->New<Bound>();
        }
    }
    return nullptr;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::DoUnaryArithmeticOp(GateRef gate)
{
    auto op = acc_.GetTypedUnAccessor(gate).GetTypedUnOp();
    auto x = FindBoundGate(acc_.GetValueIn(gate, 0));
    int constValue = 0;
    if (op == TypedUnOp::TYPED_INC) {
        constValue = 1;
    } else if (op == TypedUnOp::TYPED_DEC) {
        constValue = -1;
    } else {
        return chunk_->New<Bound>();
    }
    Bound *bound = GetBound(x);
    // only support which bounds has been calculated
    if (!bound->HasUpper() || !bound->HasLower()) {
        return chunk_->New<Bound>();
    }

    int lower = bound->Lower();
    int upper = bound->Upper();
    int newLower = lower + constValue;
    int newUpper = upper + constValue;
    bool overflow = ((constValue < 0 && (newLower > lower)) ||
                        (constValue > 0 && (newUpper < upper)));
    if (overflow) {
        return chunk_->New<Bound>();
    } else {
        return chunk_->New<Bound>(newLower, bound->LowerGate(), newUpper, bound->UpperGate());
    }
}

bool ArrayBoundsCheckElimination::InLoop(GateRef loopHeader, GateRef gate)
{
    while (gate != acc_.GetStateRoot()) {
        if (gate == loopHeader) {
            return true;
        } else {
            gate = acc_.GetState(gate, 0);
        }
    }
    return false;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::DoPhi(GateRef gate)
{
    Bound *bound = nullptr;
    size_t valueSize = acc_.GetInValueCount(gate);
    GateRef stateIn = acc_.GetState(gate);
    bool isLoopHead = acc_.IsLoopHead(stateIn);
    bool hasUpper = true;
    bool hasLower = true;
    for (size_t i = 0; i < valueSize; i++) {
        GateRef value = FindBoundGate(acc_.GetValueIn(gate, i));
        // Check if instruction is connected with phi itself
        if (isLoopHead && acc_.GetOpCode(value) == OpCode::TYPED_UNARY_OP
            && InLoop(stateIn, value)) {
            auto unOp = acc_.GetTypedUnAccessor(value).GetTypedUnOp();
            switch (unOp) {
                case TypedUnOp::TYPED_INC: {
                    hasUpper = false;
                    break;
                }
                case TypedUnOp::TYPED_DEC: {
                    hasLower = false;
                    break;
                }
                default:
                    break;
            }
            continue;
        }

        Bound *vBound = GetBound(value);
        if (vBound == nullptr) {
            LOG_ECMA(FATAL) << "ArrayBoundsCheckElimination::DoPhi:vBound is nullptr";
            UNREACHABLE();
        }
        Bound *curBound;
        GateRef curGate;
        int curConstant;
        GetInstrAndConstValueFromOp(value, curGate, curConstant);
        if (!vBound->HasUpper() || !vBound->HasLower()) {
            curBound = chunk_->New<Bound>(curConstant, curGate, curConstant, curGate);
        } else {
            curBound = vBound;
        }

        if (curBound) {
            if (!bound) {
                bound = curBound->Copy(chunk_);
            } else {
                bound = OrOp(bound, curBound);
            }
        } else {
            bound = chunk_->New<Bound>();
            break;
        }
    }

    if (!hasUpper) {
        bound->RemoveUpper();
    }
    if (!hasLower) {
        bound->RemoveLower();
    }
    return bound;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::VisitGate(GateRef gate)
{
    OpCode op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::CONSTANT:
            return DoConstant(gate);
        case OpCode::TYPED_BINARY_OP:
            return DoBinaryArithmeticOp(gate);
        case OpCode::TYPED_UNARY_OP:
            return DoUnaryArithmeticOp(gate);
        case OpCode::VALUE_SELECTOR:
            return DoPhi(gate);
        default:
            return nullptr;
    }
    return nullptr;
}

bool ArrayBoundsCheckElimination::GetInstrAndConstValueFromBinaryOp(GateRef gate, GateRef& other, int& value)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::TYPED_BINARY_OP);
    auto op = acc_.GetTypedBinaryOp(gate);
    auto x = FindBoundGate(acc_.GetValueIn(gate, 0));
    auto y = FindBoundGate(acc_.GetValueIn(gate, 1));
    other = x;
    if ((op == TypedBinOp::TYPED_ADD && (acc_.IsConstant(x) || acc_.IsConstant(y)))
        || (op == TypedBinOp::TYPED_SUB && acc_.IsConstant(y))) {
        if (acc_.IsConstant(x)) {
            value = static_cast<int>(acc_.GetConstantValue(x));
            other = y;
        } else {
            value = static_cast<int>(acc_.GetConstantValue(y));
            other = x;
        }
        if (op == TypedBinOp::TYPED_SUB) {
            value = -value;
        }
    } else {
        return false;
    }
    return true;
}

bool ArrayBoundsCheckElimination::GetInstrAndConstValueFromUnaryOp(GateRef gate, GateRef& other, int& value)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::TYPED_UNARY_OP);
    auto op = acc_.GetTypedUnAccessor(gate).GetTypedUnOp();
    auto x = FindBoundGate(acc_.GetValueIn(gate, 0));
    if (op == TypedUnOp::TYPED_INC) {
        value = 1;
        other = x;
    } else if (op == TypedUnOp::TYPED_DEC) {
        value = -1;
        other = x;
    } else {
        return false;
    }
    return true;
}

void ArrayBoundsCheckElimination::GetInstrAndConstValueFromOp(GateRef gate, GateRef& instrValue, int& constValue)
{
    int base = 0;
    constValue = 0;
    instrValue = gate;
    if (acc_.IsConstant(gate)) {
        constValue = static_cast<int>(acc_.GetConstantValue(gate));
        instrValue = Circuit::NullGate();
        return ;
    }

    while (acc_.GetOpCode(gate) == OpCode::TYPED_BINARY_OP ||
           acc_.GetOpCode(gate) == OpCode::TYPED_UNARY_OP) {
        bool flag = false;
        int value = 0;
        GateRef other = Circuit::NullGate();
        if (acc_.GetOpCode(gate) == OpCode::TYPED_BINARY_OP) {
            flag = GetInstrAndConstValueFromBinaryOp(gate, other, value);
        } else if (acc_.GetOpCode(gate) == OpCode::TYPED_UNARY_OP) {
            flag = GetInstrAndConstValueFromUnaryOp(gate, other, value);
        }
        if (!flag) {
            break;
        }
        if (acc_.IsConstant(other)) {
            base += value + static_cast<int>(acc_.GetConstantValue(other));
            constValue = base;
            instrValue = Circuit::NullGate();
            break ;
        } else {
            base += value;
            constValue = base;
            instrValue = other;
            gate = other;
        }
    }
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::GetBound(GateRef gate)
{
    if (gate == Circuit::NullGate()) {
        return nullptr;
    }
    if (!bounds_[acc_.GetId(gate)]) {
        bounds_[acc_.GetId(gate)] = chunk_->New<BoundStack>(chunk_);
        Bound *bound = VisitGate(gate);
        if (bound) {
            bounds_[acc_.GetId(gate)]->push_back(bound);
        }
        if (bounds_[acc_.GetId(gate)]->size() == 0) {
            bounds_[acc_.GetId(gate)]->push_back(chunk_->New<Bound>());
        }
    } else if (bounds_[acc_.GetId(gate)]->size() == 0) {
        return chunk_->New<Bound>();
    }
    return bounds_[acc_.GetId(gate)]->back();
}

void ArrayBoundsCheckElimination::UpdateBound(IntegerStack &pushed, GateRef gate, Bound *bound)
{
    if (acc_.IsConstant(gate)) {
        // No bound update for constants
        return;
    }
    if (!bounds_[acc_.GetId(gate)]) {
        GetBound(gate);
    }
    Bound* top = nullptr;
    if (bounds_[acc_.GetId(gate)]->size() > 0) {
        top = bounds_[acc_.GetId(gate)]->back();
    }
    if (top) {
        bound = AndOp(bound, top);
    }
    bounds_[acc_.GetId(gate)]->push_back(bound);
    pushed.push_back(acc_.GetId(gate));
}

/*
x op y + constValue
for example:
    x >= Circuit::NullGate() + 0
    x < Length + 0
*/
void ArrayBoundsCheckElimination::UpdateBound(IntegerStack &pushed, GateRef x, TypedBinOp op,
                                              GateRef instrValue, int constValue)
{
    if (op == TypedBinOp::TYPED_GREATER) { // x < 3 -> x <= 4
        op = TypedBinOp::TYPED_GREATEREQ;
        // Cannot Represent c > INT_MAX, do not update bounds
        if (constValue == INT_MAX && instrValue == Circuit::NullGate()) {
            return;
        }
        constValue++;
    } else if (op == TypedBinOp::TYPED_LESS) { // x > 3 -> x >= 2
        op = TypedBinOp::TYPED_LESSEQ;
        // Cannot Represent c < INT_MIN, do not update bounds
        if (constValue == INT_MIN && instrValue == Circuit::NullGate()) {
            return;
        }
        constValue--;
    }
    Bound *bound = chunk_->New<Bound>(op, instrValue, constValue);
    UpdateBound(pushed, x, bound);
}

// Add if condition when x is a variable, x op y
void ArrayBoundsCheckElimination::AddIfCondition(IntegerStack &pushed, GateRef x, GateRef y, TypedBinOp op)
{
    if (acc_.IsConstant(x)) { // x must be non-constant!
        return;
    }
    int constValue;
    GateRef instrValue;
    GetInstrAndConstValueFromOp(y, instrValue, constValue);
    UpdateBound(pushed, x, op, instrValue, constValue);
}

bool ArrayBoundsCheckElimination::IsArrayLength(GateRef gate)
{
    if (gate == Circuit::NullGate()) {
        return false;
    }
    OpCode op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::LOAD_ARRAY_LENGTH:
        case OpCode::LOAD_TYPED_ARRAY_LENGTH:
            return true;
        default:
            return false;
    }
    UNREACHABLE();
    return false;
}

GateRef ArrayBoundsCheckElimination::FindBoundGate(GateRef gate)
{
    OpCode op = acc_.GetOpCode(gate);
    switch (op) {
        // skip gate
        case OpCode::RANGE_GUARD:
            gate = FindBoundGate(acc_.GetValueIn(gate, 0));
            break;
        case OpCode::INDEX_CHECK: // get index
            gate = FindBoundGate(acc_.GetValueIn(gate, 1));
            break;
        default:
            break;
    }
    return gate;
}

bool ArrayBoundsCheckElimination::InArrayBound(Bound *bound, GateRef length, GateRef array)
{
    if (!bound || array == Circuit::NullGate()) {
        return false;
    }

    if (bound->Lower() >= 0 && bound->LowerGate() == Circuit::NullGate() &&
        bound->Upper() < 0 && bound->UpperGate() != Circuit::NullGate()) {
        if (length != Circuit::NullGate() && bound->UpperGate() == length) {
            return true;
        }
    }

    return false;
}

void ArrayBoundsCheckElimination::RemoveIndexCheck(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    ASSERT(acc_.GetStateCount(gate) == 1);
    ASSERT(acc_.GetInValueCount(gate) == 2); // 2: ValueCount

    GateRef depend = acc_.GetDep(gate);
    GateRef state = acc_.GetState(gate);
    GateRef value = acc_.GetValueIn(gate, 1); // Index

    acc_.ReplaceGate(gate, state, depend, value);
}

bool ArrayBoundsCheckElimination::CheckLoop(GateRef array, GateRef lowerGate, int lower, GateRef upperGate, int upper)
{
    if (IsArrayLength(upperGate) && FindBoundGate(acc_.GetValueIn(upperGate, 0)) == array) {
        if (upper >= 0) {
            return false;
        }
    }
    if (IsArrayLength(lowerGate) && FindBoundGate(acc_.GetValueIn(lowerGate, 0)) == array) {
        if (lower >= 0) {
            return false;
        }
    }
    return true;
}

bool ArrayBoundsCheckElimination::LoopInvariant(GateRegion *loopHeader, GateRef gate)
{
    if (gate == Circuit::NullGate()) {
        return true;
    }
    auto gateRegion = graphLinearizer_.GateToRegion(gate);
    GateRegion* g = loopHeader->GetDominator();
    while (g != nullptr) {
        if (g == gateRegion) {
            return true;
        }
        if (g == g->GetDominator()) { // entry
            break ;
        }
        g = g->GetDominator();
    }
    return false;
}

GateRef ArrayBoundsCheckElimination::Predicate(GateRef left, TypedBinOp cond, GateRef right)
{
    return builder_.InsertRangeCheckPredicate(left, cond, right);
}

GateRef ArrayBoundsCheckElimination::PredicateCmpWithConst(GateRef left, TypedBinOp cond, int32_t right)
{
    GateRef constGate = builder_.Int32(right);
    return Predicate(left, cond, constGate);
}

GateRef ArrayBoundsCheckElimination::PredicateAdd(GateRef left, int32_t leftConst, TypedBinOp cond, GateRef right)
{
    GateRef constGate = builder_.Int32(leftConst);
    GateRef binaryOpGate = builder_.InsertTypedBinaryop(left, constGate, TypedBinOp::TYPED_ADD);
    return Predicate(binaryOpGate, cond, right);
}

GateRef ArrayBoundsCheckElimination::PredicateAddCmpWithConst(GateRef left, int32_t leftConst,
                                                              TypedBinOp cond, int32_t right)
{
    GateRef constGate = builder_.Int32(right);
    return PredicateAdd(left, leftConst, cond, constGate);
}

void ArrayBoundsCheckElimination::LoopInvariantMotionForIndexCheck(GateRef array, GateRef length,
    GateRef lengthMetaData, GateRef lowerGate, int lower, GateRef upperGate, int upper, bool isTypedArray)
{
    // lower > 0
    if (lowerGate != Circuit::NullGate()) {
        if (lower == 0) {
            // lowerGate >= 0
            PredicateCmpWithConst(lowerGate, TypedBinOp::TYPED_GREATEREQ, 0);
        } else if (lower > 0) {
            // lowerGate + lower >= 0
            PredicateAddCmpWithConst(lowerGate, lower, TypedBinOp::TYPED_GREATEREQ, 0);
        } else {
            // lowerGate + lower < 0
            // lower < 0
            // lowerGate < -lower
            lower++;
            lower = -lower;
            PredicateCmpWithConst(lowerGate, TypedBinOp::TYPED_GREATER, lower);
        }
    }

    // LOAD LENGTH if necessary
    if (length == Circuit::NullGate()) {
        length = builder_.InsertLoadArrayLength(array, lengthMetaData, isTypedArray);
    }

    if (upperGate == Circuit::NullGate()) {
        ASSERT(upper >= 0);
        PredicateCmpWithConst(length, TypedBinOp::TYPED_GREATER, upper);
    } else {
        if (upper == 0) {
            Predicate(upperGate, TypedBinOp::TYPED_LESS, length);
        } else if (upper > 0) {
            // upperGate + upper < length
            PredicateAdd(upperGate, upper, TypedBinOp::TYPED_LESS, length);
        } else {
            // upperGate + upper < length
            // upper < 0
            // upperGate < length + (-upper)
            PredicateAdd(length, -upper, TypedBinOp::TYPED_GREATER, upperGate);
        }
    }
}

void ArrayBoundsCheckElimination::ProcessIndexCheck(GateRegion *loopHeader, GateRef gate)
{
    auto length = FindBoundGate(acc_.GetValueIn(gate, 0));
    auto array = FindBoundGate(acc_.GetValueIn(length, 0));
    auto index = FindBoundGate(acc_.GetValueIn(gate, 1));
    Bound *indexBound = GetBound(index);
    if (!indexBound->HasLower() || !indexBound->HasUpper()) {
        return;
    }

    if (InArrayBound(indexBound, length, array)) {
        RemoveIndexCheck(gate);
    } else if (loopHeader) {
        if (!LoopInvariant(loopHeader, array)
            || !LoopInvariant(loopHeader, indexBound->LowerGate())
            || !LoopInvariant(loopHeader, indexBound->UpperGate())
            || (indexBound->LowerGate() == Circuit::NullGate() && indexBound->Lower() < 0)
            || (indexBound->UpperGate() == Circuit::NullGate() && indexBound->Upper() < 0)) {
            return;
        }

        ASSERT(length != Circuit::NullGate());
        GateRef lengthMetaData = length;
        bool isTypedArray = false;
        if (acc_.GetOpCode(length) == OpCode::LOAD_TYPED_ARRAY_LENGTH) {
            isTypedArray = true;
        }

        // Length instrution
        if (!LoopInvariant(loopHeader, length)) {
            // Generate length instruction yourself
            length = Circuit::NullGate();
        }

        // Insert Before loopHeader State, and if find IF_TRUE and IF_FALSE, insert after the DEPEND_RELAY
        // if find MERGE, insert after DEPEND_SELECTOR
        GateRef insertAfter = acc_.GetState(loopHeader->GetState(), 0); // after end
        GateRef stateIn = insertAfter;
        GateRef dependIn = insertAfter;
        acc_.GetStateInAndDependIn(insertAfter, stateIn, dependIn);

        if (!CheckLoop(array, indexBound->LowerGate(), indexBound->Lower(),
                       indexBound->UpperGate(), indexBound->Upper())) {
            return;
        }

        Environment env(stateIn, dependIn, {}, circuit_, &builder_);
        LoopInvariantMotionForIndexCheck(array, length, lengthMetaData, indexBound->LowerGate(),
            indexBound->Lower(), indexBound->UpperGate(), indexBound->Upper(), isTypedArray);
        RemoveIndexCheck(gate);
    }
}

void ArrayBoundsCheckElimination::ProcessIf(IntegerStack &pushed, GateRegion *parent, OpCode cond)
{
    auto& gateLists = parent->GetGates();
    for (int i = static_cast<int>(gateLists.size()) - 1; i >= 0; i--) { // Found the last BinaryOp
        GateRef gate = gateLists[i];
        if (gate == Circuit::NullGate()) continue;
        OpCode opGate = acc_.GetOpCode(gate);
        if (opGate != OpCode::TYPED_BINARY_OP) {
            continue ;
        }

        TypedBinOp op = acc_.GetTypedBinaryOp(gate);
        GateRef x = FindBoundGate(acc_.GetValueIn(gate, 0));
        GateRef y = FindBoundGate(acc_.GetValueIn(gate, 1));

        switch (op) {
            case TypedBinOp::TYPED_LESS:
            case TypedBinOp::TYPED_LESSEQ:
            case TypedBinOp::TYPED_GREATER:
            case TypedBinOp::TYPED_GREATEREQ:
            case TypedBinOp::TYPED_EQ:
            case TypedBinOp::TYPED_NOTEQ: {
                if (cond == OpCode::IF_TRUE) {
                    op = acc_.GetRevCompareOpForTypedBinOp(op);
                }
                AddIfCondition(pushed, x, y, op);
                AddIfCondition(pushed, y, x, acc_.GetSwapCompareOpForTypedBinOp(op));
                break;
            }
            default:
                break;
        }
        break;
    }
}

bool ArrayBoundsCheckElimination::Contain(GateLists &gateLists, GateRef gate)
{
    for (size_t i = 0; i < gateLists.size(); i++) {
        if (gateLists[i] == gate) {
            return true;
        }
    }
    return false;
}

void ArrayBoundsCheckElimination::AddAccessIndexedInfo(GateLists &indices, GateRef gate, int idx, GateRef indexCheck)
{
    IndexCheckInfo *indexCheckInfo = indexCheckInfo_[acc_.GetId(gate)];
    if (indexCheckInfo == nullptr) {
        indexCheckInfo = chunk_->New<IndexCheckInfo>(chunk_);
        indexCheckInfo_[acc_.GetId(gate)] = indexCheckInfo;
        indices.push_back(gate);
        indexCheckInfo->min_ = idx;
        indexCheckInfo->max_ = idx;
    } else if (idx >= indexCheckInfo->min_ && idx <= indexCheckInfo->max_) {
        RemoveIndexCheck(indexCheck);
        return;
    }
    indexCheckInfo->min_ = std::min(indexCheckInfo->min_, idx);
    indexCheckInfo->max_ = std::max(indexCheckInfo->max_, idx);
    indexCheckInfo->list_.push_back(indexCheck);
}

void ArrayBoundsCheckElimination::InBlockMotion(GateLists &indexChecked, GateLists &arrays)
{
    GateLists indices(chunk_);
    for (size_t i = 0; i < arrays.size(); i++) {
        int maxConstant = -1;
        GateLists listConstant(chunk_);
        GateRef arrayGate = arrays[i];
        for (size_t j = 0; j < indexChecked.size(); j++) {
            GateRef indexCheck = indexChecked[j];
            // INDEX_CHECK may be dead
            if (acc_.GetOpCode(indexCheck) != OpCode::INDEX_CHECK) {
                continue;
            }
            GateRef length = FindBoundGate(acc_.GetValueIn(indexCheck, 0));
            GateRef index = FindBoundGate(acc_.GetValueIn(indexCheck, 1));
            GateRef array = FindBoundGate(acc_.GetValueIn(length, 0));
            if (array != arrayGate) {
                continue;
            }
            if (acc_.IsConstant(index)) {
                int constValue = static_cast<int>(acc_.GetConstantValue(index));
                if (constValue >= 0 && constValue <= maxConstant) {
                    RemoveIndexCheck(indexCheck);
                } else if (constValue >= 0 && constValue > maxConstant) {
                    maxConstant = constValue;
                    listConstant.push_back(indexCheck);
                }
            } else {
                int lastInteger;
                GateRef lastGate;
                GetInstrAndConstValueFromOp(index, lastGate, lastInteger);
                if (lastInteger >= 0 && lastGate == Circuit::NullGate()) { // IsConstant
                    if (lastInteger <= maxConstant) {
                        RemoveIndexCheck(indexCheck);
                    } else {
                        maxConstant = lastInteger;
                        listConstant.push_back(indexCheck);
                    }
                } else if (lastGate != Circuit::NullGate()) {
                    AddAccessIndexedInfo(indices, lastGate, lastInteger, indexCheck);
                } // when lastInteger < 0, dont remove IndexCheck
            }
        }

        // Iterate over all different indices
        for (size_t j = 0; j < indices.size(); j++) {
            GateRef index = indices[j];

            IndexCheckInfo *info = indexCheckInfo_[acc_.GetId(index)];
            ASSERT(info != nullptr);

            // maybe index < 0, max > 0
            // max + index in [0, a.length)
            // min + index overflow !!!, min + index > 0
            // so, min + index >= INT_MIN, min >= INT_MIN - index
            // max in [-index, a.length - index)
            // min >= INT_MIN + max
            bool rangeCond = (info->max_ < 0 || info->max_ + INT_MIN <= info->min_);
            if (info->list_.size() > 2 && rangeCond) { // 2: size
                GateRef insertAfter = info->list_.front();
                GateRef length = FindBoundGate(acc_.GetValueIn(insertAfter, 0));
                ASSERT(length != Circuit::NullGate());

                Environment env(insertAfter, circuit_, &builder_);

                // Calculate lower bound
                GateRef lowerCompare = index;
                if (info->min_ > 0) {
                    GateRef minGate = builder_.Int32(info->min_);
                    lowerCompare = builder_.InsertTypedBinaryop(lowerCompare, minGate, TypedBinOp::TYPED_ADD);
                } else if (info->min_ < 0) {
                    GateRef minGate = builder_.Int32(-info->min_);
                    lowerCompare = builder_.InsertTypedBinaryop(lowerCompare, minGate, TypedBinOp::TYPED_SUB);
                }

                PredicateCmpWithConst(lowerCompare, TypedBinOp::TYPED_GREATEREQ, 0);

                // Calculate upper bound
                GateRef upperCompare = index;
                if (info->max_ != 0) {
                    if (info->max_ > 0) {
                        GateRef maxGate = builder_.Int32(info->max_);
                        upperCompare = builder_.InsertTypedBinaryop(upperCompare, maxGate, TypedBinOp::TYPED_ADD);
                    } else if (info->max_ < 0) {
                        GateRef maxGate = builder_.Int32(-info->max_);
                        upperCompare = builder_.InsertTypedBinaryop(upperCompare, maxGate, TypedBinOp::TYPED_SUB);
                    }
                }

                Predicate(upperCompare, TypedBinOp::TYPED_LESS, length);
                for (auto& indexCheck: (info->list_)) {
                    RemoveIndexCheck(indexCheck);
                }
            }
        }

        // index only constant
        if (listConstant.size() > 1) {
            GateRef firIndexCheckGate = listConstant.front();
            Environment env(firIndexCheckGate, circuit_, &builder_);
            GateRef length = FindBoundGate(acc_.GetValueIn(firIndexCheckGate, 0));
            ASSERT(length != Circuit::NullGate());
            ASSERT(maxConstant >= 0);
            PredicateCmpWithConst(length, TypedBinOp::TYPED_GREATER, maxConstant); // length > index
            for (size_t j = 0; j < listConstant.size(); j++) {
                GateRef indexCheck = listConstant[j];
                RemoveIndexCheck(indexCheck);
            }
        }

        for (size_t j = 0; j < indices.size(); j++) {
            indexCheckInfo_[acc_.GetId(indices[j])] = nullptr;
        }
        indices.clear();
    }
}

void ArrayBoundsCheckElimination::CalcBounds(GateRegion *block, GateRegion *loopHeader)
{
    // Pushed stack for condition
    IntegerStack pushed(chunk_);

    // Process If
    GateRegion *parent = block->GetDominator();
    if (parent != nullptr) {
        auto gate = block->GetGates().front();
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::IF_TRUE || op == OpCode::IF_FALSE) { // Recognize If (including the condition in forloop)
            ProcessIf(pushed, parent, op);
        }
    }

    GateLists indexChecked(chunk_);
    GateLists arrays(chunk_);

    auto& gateList_ = block->GetGates();
    for (size_t i = 0; i < gateList_.size(); i++) { // Visit GateUnion
        GateRef gate = gateList_[i];
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::INDEX_CHECK) {
            auto length = FindBoundGate(acc_.GetValueIn(gate, 0));
            auto index = FindBoundGate(acc_.GetValueIn(gate, 1));
            auto array = FindBoundGate(acc_.GetValueIn(length, 0));

            ProcessIndexCheck(loopHeader, gate);
            indexChecked.push_back(gate);

            if (!Contain(arrays, array)) {
                arrays.push_back(array);
            }

            // Give IndexCheck a bound [0, Length - 1]
            Bound *b = GetBound(index);
            if (b->LowerGate() == Circuit::NullGate()) { // LowerBound is the Constant !!!
                UpdateBound(pushed, index, TypedBinOp::TYPED_GREATEREQ, Circuit::NullGate(), 0);
            }
            if (!b->HasUpper() && length != Circuit::NullGate()) { // default dont know the Length
                UpdateBound(pushed, index, TypedBinOp::TYPED_LESS, length, 0);
            }
        }
    }

    InBlockMotion(indexChecked, arrays);

    auto& dominatedRegions_ = block->GetDominatedRegions();
    for (size_t i = 0; i < dominatedRegions_.size(); i++) {
        GateRegion *nex = dominatedRegions_[i];
        if (block->IsLoopHead() && (block->GetLoopIndex() == nex->GetLoopIndex()
            || nex->GetLoopDepth() > block->GetLoopDepth())) {
            CalcBounds(nex, block);
        } else {
            CalcBounds(nex, loopHeader);
        }
    }

    for (size_t i = 0; i < pushed.size(); i++) {
        bounds_[pushed[i]]->pop_back();
    }
}
}
