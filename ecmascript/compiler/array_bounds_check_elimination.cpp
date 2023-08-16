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
    graphLinearizer_.SetScheduleJSOpcode();
    graphLinearizer_.LinearizeGraph();
    
    CalcBounds(graphLinearizer_.GetEntryRegion());

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After array bounds check elimination "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
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
        case TypedBinOp::TYPED_EQ:
            lower_ = constant;
            lowerGate_ = gate;
            upper_ = constant;
            upperGate_ = gate;
            break;
        case TypedBinOp::TYPED_NOTEQ:
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
        case TypedBinOp::TYPED_GREATEREQ:
            lower_ = constant;
            lowerGate_ = gate;
            upper_ = INT_MAX;
            upperGate_ = Circuit::NullGate();
            break;
        case TypedBinOp::TYPED_LESSEQ:
            lower_ = INT_MIN;
            lowerGate_ = Circuit::NullGate();
            upper_ = constant;
            upperGate_ = gate;
            break;
        default:
            UNREACHABLE();
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
            set = (boundLowerDominatorDepth > bLowerDominatorDepth);
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
            set = (boundUpperDominatorDepth > bUpperDominatorDepth);
        }
        if (set) {
            bound->upper_ = b->upper_;
            bound->upperGate_ = b->upperGate_;
        }
    }
    
    return bound;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::DoConstant(GateRef gate)
{
    int constValue = acc_.GetConstantValue(gate);
    return new Bound(constValue, Circuit::NullGate(), constValue, Circuit::NullGate());
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::DoArithmeticOp(GateRef gate)
{
    auto op = acc_.GetTypedBinaryOp(gate);
    auto x = acc_.GetValueIn(gate, 0);
    auto y = acc_.GetValueIn(gate, 1);
    if (!acc_.IsConstant(x) || !acc_.IsConstant(y)) { // One of the operands must be non-constant!
        if (((acc_.IsConstant(x) || acc_.IsConstant(y)) && op == TypedBinOp::TYPED_ADD) ||
            (acc_.IsConstant(y) && op == TypedBinOp::TYPED_SUB)) {
            // x is constant, y is variable.
            if (acc_.IsConstant(y)) {
                std::swap(x, y);
            }

            // Add, Constant now in x
            int constValue = acc_.GetConstantValue(x);
            if (op == TypedBinOp::TYPED_SUB) {
                constValue = -constValue;
            }

            Bound *bound = GetBound(y);
            if (!bound->HasUpper() || !bound->HasLower()) {
                return new Bound();
            }

            int lower = bound->Lower();
            int upper = bound->Upper();
            int newLower = lower + constValue;
            int newUpper = upper + constValue;
            bool overflow = ((constValue < 0 && (newLower > lower)) ||
                                (constValue > 0 && (newUpper < upper)));
            if (overflow) {
                return new Bound();
            } else {
                return new Bound(newLower, bound->LowerGate(), newUpper, bound->UpperGate());
            }
        } else {
            Bound *bound = GetBound(x);
            if (op == TypedBinOp::TYPED_SUB && bound->LowerGate() == y) {
                return new Bound(TypedBinOp::TYPED_GREATEREQ, Circuit::NullGate(), bound->Lower());
            } else {
                return new Bound();
            }
        }
    }
    return nullptr;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::VisitGate(GateRef gate)
{
    OpCode op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::CONSTANT:
            return DoConstant(gate);
        case OpCode::TYPED_BINARY_OP:
            return DoArithmeticOp(gate);
        default:
            return nullptr;
    }
    return nullptr;
}

ArrayBoundsCheckElimination::Bound *ArrayBoundsCheckElimination::GetBound(GateRef gate)
{
    if (gate == Circuit::NullGate()) {
        return nullptr;
    }
    if (!bounds_[acc_.GetId(gate)]) {
        bounds_[acc_.GetId(gate)] = new BoundStack(chunk_);
        Bound *bound = VisitGate(gate);
        if (bound) {
            bounds_[acc_.GetId(gate)]->push_back(bound);
        }
        if (bounds_[acc_.GetId(gate)]->size() == 0) {
            bounds_[acc_.GetId(gate)]->push_back(new Bound());
        }
    } else if (bounds_[acc_.GetId(gate)]->size() == 0) {
        return new Bound();
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
        } else {
            constValue++;
        }
    } else if (op == TypedBinOp::TYPED_LESS) { // x > 3 -> x >= 2
        op = TypedBinOp::TYPED_LESSEQ;
        // Cannot Represent c < INT_MIN, do not update bounds
        if (constValue == INT_MIN && instrValue == Circuit::NullGate()) {
            return;
        } else {
            constValue--;
        }
    }
    Bound *bound = new Bound(op, instrValue, constValue);
    UpdateBound(pushed, x, bound);
}

// Add if condition when x is a variable, x op y
void ArrayBoundsCheckElimination::AddIfCondition(IntegerStack &pushed, GateRef x, GateRef y, TypedBinOp op)
{
    if (acc_.IsConstant(x)) { // x must be non-constant!
        return;
    }
    int constValue = 0;
    GateRef instrValue = y;
    if (acc_.IsConstant(y)) {
        constValue = acc_.GetConstantValue(y);
        instrValue = Circuit::NullGate();
    } else if (acc_.GetOpCode(y) == OpCode::TYPED_BINARY_OP) {
        auto binaryOp = acc_.GetTypedBinaryOp(y);
        auto a = acc_.GetValueIn(y, 0);
        auto b = acc_.GetValueIn(y, 1);
        if (binaryOp == TypedBinOp::TYPED_ADD) {
            if (acc_.IsConstant(a)) {
                constValue = acc_.GetConstantValue(a);
                instrValue = b;
            } else if (acc_.IsConstant(b)) {
                constValue = acc_.GetConstantValue(b);
                instrValue = a;
            }
        } else if (binaryOp == TypedBinOp::TYPED_SUB && acc_.IsConstant(b)) { // x >= a op b
            constValue = -acc_.GetConstantValue(b);
            instrValue = a;
        }
    }
    UpdateBound(pushed, x, op, instrValue, constValue);
}

bool ArrayBoundsCheckElimination::InArrayBound(Bound *bound, GateRef length, GateRef array)
{
    if (!bound || array == Circuit::NullGate()) {
        return false;
    }
    if (bound->Lower() >= 0 && bound->LowerGate() == Circuit::NullGate() &&
       bound->Upper() < 0 && bound->UpperGate() != Circuit::NullGate()) {
        if (bound->UpperGate() == array || (length != Circuit::NullGate() && bound->UpperGate() == length)) {
            return true;
        }
    }
    return false;
}

void ArrayBoundsCheckElimination::RemoveIndexCheck(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    ASSERT(acc_.GetStateCount(gate) == 1);
    ASSERT(acc_.GetInValueCount(gate) == 2);
    
    GateRef depend = acc_.GetDep(gate);
    GateRef state = acc_.GetState(gate);
    GateRef value = acc_.GetValueIn(gate, 1); // Index

    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.IsStateIn(it)) {
            ASSERT(state != Circuit::NullGate());
            it = acc_.ReplaceIn(it, state);
        } else if (acc_.IsDependIn(it)) {
            ASSERT(depend != Circuit::NullGate());
            it = acc_.ReplaceIn(it, depend);
        } else if (acc_.IsValueIn(it)) {
            ASSERT(value != Circuit::NullGate());
            it = acc_.ReplaceIn(it, value);
        } else {
            ++it;
        }
    }
    acc_.DeleteGate(gate);
}

void ArrayBoundsCheckElimination::ProcessIndexCheck(GateRef gate)
{
    auto length = acc_.GetValueIn(gate, 0);
    auto array = acc_.GetValueIn(length, 0);
    auto index = acc_.GetValueIn(gate, 1);
    Bound *indexBound = GetBound(index);
    if (!indexBound->HasLower() || !indexBound->HasUpper()) {
        return;
    }
    if (InArrayBound(indexBound, array, length)) {
        RemoveIndexCheck(gate);
    }
}

void ArrayBoundsCheckElimination::ProcessIf(IntegerStack &pushed, GateRegion *parent, OpCode cond)
{
    auto& gateLists = parent->GetGates();
    for (size_t i = gateLists.size() - 1; i >= 0; i--) { // Found the last BinaryOp
        GateRef gate = gateLists[i];
        OpCode opGate = acc_.GetOpCode(gate);
        if (opGate != OpCode::TYPED_BINARY_OP) continue;

        TypedBinOp op = acc_.GetTypedBinaryOp(gate);
        GateRef x = acc_.GetValueIn(gate, 0);
        GateRef y = acc_.GetValueIn(gate, 1);

        switch (op) {
            case TypedBinOp::TYPED_LESS:
            case TypedBinOp::TYPED_LESSEQ:
            case TypedBinOp::TYPED_GREATER:
            case TypedBinOp::TYPED_GREATEREQ:
            case TypedBinOp::TYPED_EQ:
            case TypedBinOp::TYPED_NOTEQ:
                if (cond == OpCode::IF_TRUE) {
                    op = GateMetaData::GetRevCompareOp(op);
                }
                AddIfCondition(pushed, x, y, op);
                AddIfCondition(pushed, y, x, GateMetaData::GetSwapCompareOp(op));
                break;
            default:
                break;
        }
        break;
    }
}

bool ArrayBoundsCheckElimination::Contain(ChunkVector<GateRef>& gateLists, GateRef gate)
{
    for (size_t i = 0; i < gateLists.size(); i++) {
        if (gateLists[i] == gate) {
            return true;
        }
    }
    return false;
}

void ArrayBoundsCheckElimination::CalcBounds(GateRegion *block)
{
    // Pushed stack for condition
    IntegerStack pushed(chunk_);

    // Process If
    GateRegion *parent = block->GetDominator();
    if (parent != nullptr) {
        auto gate = block->GetGates().front();
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::IF_TRUE || op == OpCode::IF_FALSE) { // Recognize If
            ProcessIf(pushed, parent, op);
        }
    }
    
    GateLists IndexChecked(chunk_);
    GateLists Arrays(chunk_);

    auto& gateList_ = block->GetGates();
    for (size_t i = 0; i < gateList_.size(); i++) { // Visit GateUnion
        GateRef gate = gateList_[i];
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::INDEX_CHECK) {
            auto length = acc_.GetValueIn(gate, 0);
            auto index = acc_.GetValueIn(gate, 1);
            auto array = acc_.GetValueIn(length, 0);

            ProcessIndexCheck(gate);
            IndexChecked.push_back(gate);

            if (!Contain(Arrays, array)) {
                Arrays.push_back(array);
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
    
    auto& dominatedRegions_ = block->GetDominatedRegions();
    for (size_t i = 0; i < dominatedRegions_.size(); i++) {
        GateRegion *nex = dominatedRegions_[i];
        CalcBounds(nex);
    }

    for (size_t i = 0; i < pushed.size(); i++) {
        bounds_[pushed[i]]->pop_back();
    }
}
}