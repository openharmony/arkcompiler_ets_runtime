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

#include "ecmascript/compiler/range_analysis.h"

namespace panda::ecmascript::kungfu {

GateRef RangeAnalysis::UpdateRange(GateRef gate, const RangeInfo& info)
{
    auto &range = rangeInfos_[acc_.GetId(gate)];
    if (range != info) {
        range = info;
        return gate;
    } else {
        return Circuit::NullGate();
    }
}

RangeInfo RangeAnalysis::GetRange(GateRef gate) const
{
    ASSERT(acc_.GetId(gate) < rangeInfos_.size());
    return rangeInfos_[acc_.GetId(gate)];
}

bool RangeAnalysis::IsInt32Type(GateRef gate) const
{
    auto id = acc_.GetId(gate);
    if (id >= typeInfos_.size()) {
        return acc_.GetMachineType(gate) == MachineType::I32;
    }
    return typeInfos_[id] == TypeInfo::INT32;
}

GateRef RangeAnalysis::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::CONSTANT:
            return VisitConstant(gate);
        case OpCode::VALUE_SELECTOR:
            return VisitPhi(gate);
        case OpCode::TYPED_BINARY_OP:
            return VisitTypedBinaryOp(gate);
        case OpCode::TYPED_UNARY_OP:
            return VisitTypedUnaryOp(gate);
        case OpCode::INDEX_CHECK:
            return VisitIndexCheck(gate);
        case OpCode::LOAD_ARRAY_LENGTH:
            return VisitLoadArrayLength(gate);
        case OpCode::LOAD_STRING_LENGTH:
            return VisitLoadStringLength(gate);
        case OpCode::LOAD_MAP_SIZE:
            return VisitLoadMapSize(gate);
        case OpCode::LOAD_TYPED_ARRAY_LENGTH:
            return VisitLoadTypedArrayLength(gate);
        case OpCode::RANGE_GUARD:
            return VisitRangeGuard(gate);
        default:
            return VisitOthers(gate);
    }
}

GateRef RangeAnalysis::VisitPhi(GateRef gate)
{
    if (!IsInt32Type(gate)) {
        return Circuit::NullGate();
    }
    auto range = RangeInfo::NONE();
    auto numIn = acc_.GetInValueCount(gate);
    for (size_t i = 0; i < numIn; ++i) {
        auto valueIn = acc_.GetValueIn(gate, i);
        range = range.Union(GetRange(valueIn));
    }
    return UpdateRange(gate, range);
}

GateRef RangeAnalysis::VisitOthers(GateRef gate)
{
    if (!IsInt32Type(gate)) {
        return Circuit::NullGate();
    }
    return UpdateRange(gate, RangeInfo::ANY());
}

GateRef RangeAnalysis::VisitConstant(GateRef gate)
{
    if (!IsInt32Type(gate)) {
        return Circuit::NullGate();
    }
    auto value = acc_.GetInt32FromConstant(gate);
    return UpdateRange(gate, RangeInfo(value, value));
}

GateRef RangeAnalysis::VisitTypedUnaryOp(GateRef gate)
{
    if (!IsInt32Type(gate)) {
        return Circuit::NullGate();
    }
    auto op = acc_.GetTypedUnAccessor(gate).GetTypedUnOp();
    auto range = GetRange(acc_.GetValueIn(gate, 0));
    if (range.IsNone()) {
        return Circuit::NullGate();
    }
    switch (op) {
        case TypedUnOp::TYPED_INC:
            range = range + RangeInfo(1, 1);
            break;
        case TypedUnOp::TYPED_DEC:
            range = range - RangeInfo(1, 1);
            break;
        case TypedUnOp::TYPED_NEG:
            range = RangeInfo(0, 0) - range;
            break;
        case TypedUnOp::TYPED_NOT:
            range = ~ range;
            break;
        default:
            break;
    }
    return UpdateRange(gate, range);
}

GateRef RangeAnalysis::VisitTypedBinaryOp(GateRef gate)
{
    if (!IsInt32Type(gate)) {
        return Circuit::NullGate();
    }
    auto op = acc_.GetTypedBinaryOp(gate);
    auto range = RangeInfo::ANY();
    switch (op) {
        case TypedBinOp::TYPED_ADD:
            range = GetRangeOfCalculate<TypedBinOp::TYPED_ADD>(gate);
            break;
        case TypedBinOp::TYPED_SUB:
            range = GetRangeOfCalculate<TypedBinOp::TYPED_SUB>(gate);
            break;
        case TypedBinOp::TYPED_MOD:
            range = GetRangeOfCalculate<TypedBinOp::TYPED_MOD>(gate);
            break;
        case TypedBinOp::TYPED_MUL:
            range = GetRangeOfCalculate<TypedBinOp::TYPED_MUL>(gate);
            break;
        case TypedBinOp::TYPED_SHR:
            range = GetRangeOfShift<TypedBinOp::TYPED_SHR>(gate);
            break;
        case TypedBinOp::TYPED_ASHR:
            range = GetRangeOfShift<TypedBinOp::TYPED_ASHR>(gate);
            break;
        default:
            break;
    }
    return UpdateRange(gate, range);
}

GateRef RangeAnalysis::VisitIndexCheck(GateRef gate)
{
    ASSERT(IsInt32Type(gate));
    auto value = GetRange(acc_.GetValueIn(gate, 0));
    auto largerRange = RangeInfo(0, INT32_MAX - 1);
    auto intersected = value.intersection(largerRange);
    return UpdateRange(gate, intersected);
}

GateRef RangeAnalysis::VisitLoadArrayLength(GateRef gate)
{
    ASSERT(IsInt32Type(gate));
    return UpdateRange(gate, RangeInfo(0, INT32_MAX));
}

GateRef RangeAnalysis::VisitLoadStringLength(GateRef gate)
{
    ASSERT(IsInt32Type(gate));
    return UpdateRange(gate, RangeInfo(0, INT32_MAX));
}

GateRef RangeAnalysis::VisitLoadMapSize(GateRef gate)
{
    ASSERT(IsInt32Type(gate));
    return UpdateRange(gate, RangeInfo(0, INT32_MAX));
}

GateRef RangeAnalysis::VisitLoadTypedArrayLength(GateRef gate)
{
    TypedArrayMetaDataAccessor accessor = acc_.GetTypedArrayMetaDataAccessor(gate);
    OnHeapMode onHeap = accessor.GetOnHeapMode();
    int32_t max = onHeap == OnHeapMode::ON_HEAP ? RangeInfo::TYPED_ARRAY_ONHEAP_MAX : INT32_MAX;
    return UpdateRange(gate, RangeInfo(0, max));
}

GateRef RangeAnalysis::VisitRangeGuard(GateRef gate)
{
    auto left = acc_.GetFirstValue(gate);
    auto right = acc_.GetSecondValue(gate);
    return UpdateRange(gate, RangeInfo(left, right));
}

template<TypedBinOp Op>
RangeInfo RangeAnalysis::GetRangeOfCalculate(GateRef gate)
{
    auto left = GetRange(acc_.GetValueIn(gate, 0));
    auto right = GetRange(acc_.GetValueIn(gate, 1));
    if (left.IsNone() || right.IsNone()) {
        return RangeInfo::NONE();
    }
    switch (Op) {
        case TypedBinOp::TYPED_ADD:
            return left + right;
        case TypedBinOp::TYPED_SUB:
            return left - right;
        case TypedBinOp::TYPED_MOD:
            return left % right;
        case TypedBinOp::TYPED_MUL:
            return left * right;
        default:
            return RangeInfo::ANY();
    }
}

template<TypedBinOp Op>
RangeInfo RangeAnalysis::GetRangeOfShift(GateRef gate)
{
    ASSERT((Op == TypedBinOp::TYPED_SHR) || (Op == TypedBinOp::TYPED_ASHR));
    auto value = GetRange(acc_.GetValueIn(gate, 0));
    auto shift = GetRange(acc_.GetValueIn(gate, 1));
    if (value.IsNone() || shift.IsNone()) {
        return RangeInfo::NONE();
    }
    if (shift.GetMin() != shift.GetMax()) {
        return RangeInfo::ANY();
    }
    switch (Op) {
        case TypedBinOp::TYPED_SHR:
            return value.SHR(shift);
        case TypedBinOp::TYPED_ASHR:
            return value.ASHR(shift);
        default:
            return RangeInfo::ANY();
    }
}

RangeInfo RangeAnalysis::TryGetRangeOfBranch(GateRef state, GateRef value)
{
    auto jmp = acc_.GetState(state);
    if (acc_.GetOpCode(jmp) == OpCode::JS_BYTECODE) {
        return GetRange(value);
    }
    ASSERT((acc_.GetOpCode(jmp) == OpCode::IF_BRANCH) || (acc_.GetOpCode(jmp) == OpCode::TYPED_CONDITION_JUMP));
    auto condition = acc_.GetValueIn(jmp);
    auto range = GetRange(value);
    if (acc_.GetOpCode(condition) != OpCode::TYPED_BINARY_OP) {
        return range;
    }
    if ((acc_.GetValueIn(condition, 0) != value) && (acc_.GetValueIn(condition, 1) != value)) {
        return range;
    }

    // flag = !(jnez ^ if_false) = jnez ^ if_true
    bool flag = acc_.GetOpCode(state) == OpCode::IF_TRUE;
    if (acc_.GetOpCode(jmp) == OpCode::TYPED_CONDITION_JUMP) {
        flag = flag != (acc_.GetTypedJumpAccessor(jmp).GetTypedJumpOp() == TypedJumpOp::TYPED_JNEZ);
    }
    return range.intersection(GetRangeOfCompare(condition, value, flag));
}

RangeInfo RangeAnalysis::GetRangeOfCompare(GateRef gate, GateRef value, bool flag)
{
    auto op = acc_.GetTypedBinaryOp(gate);
    auto left = acc_.GetValueIn(gate, 0);
    auto right = acc_.GetValueIn(gate, 1);
    ASSERT((left == value) || (right == value));
    bool swap = right == value;
    if (flag) {
        op = acc_.GetRevCompareOpForTypedBinOp(op);
    }
    if (swap) {
        op = acc_.GetSwapCompareOpForTypedBinOp(op);
    }
    auto range = GetRange(swap ? left : right);
    if (range.IsNone()) {
        // provide no info for branch range infer.
        return RangeInfo::ANY();
    }
    switch (op) {
        case TypedBinOp::TYPED_LESS:
            return RangeInfo(INT32_MIN, range.GetMax() - 1);
        case TypedBinOp::TYPED_LESSEQ:
            return RangeInfo(INT32_MIN, range.GetMax());
        case TypedBinOp::TYPED_GREATER:
            return RangeInfo(range.GetMin() + 1, INT32_MAX);
        case TypedBinOp::TYPED_GREATEREQ:
            return RangeInfo(range.GetMin(), INT32_MAX);
        case TypedBinOp::TYPED_EQ:
            return range;
        case TypedBinOp::TYPED_NOTEQ:
            return RangeInfo::ANY();
        default:
            UNREACHABLE();
            return RangeInfo::ANY();
    }
}

void RangeAnalysis::PrintRangeInfo() const
{
    std::vector<GateRef> gateList;
    acc_.GetAllGates(gateList);
    std::string log = "";
    for (auto gate : gateList) {
        if (!IsInt32Type(gate)) {
            continue;
        }
        log = "id:" + std::to_string(acc_.GetId(gate));
        auto op = acc_.GetOpCode(gate);
        switch (op) {
            case OpCode::CONSTANT: {
                log += " constant";
                break;
            }
            case OpCode::VALUE_SELECTOR: {
                log += " phi";
                break;
            }
            case OpCode::TYPED_BINARY_OP: {
                auto binOp = acc_.GetTypedBinaryOp(gate);
                switch (binOp) {
                    case TypedBinOp::TYPED_ADD:
                        log += " add";
                        break;
                    case TypedBinOp::TYPED_SUB:
                        log += " sub";
                        break;
                    case TypedBinOp::TYPED_SHR:
                        log += " shr";
                        break;
                    case TypedBinOp::TYPED_ASHR:
                        log += " ashr";
                        break;
                    case TypedBinOp::TYPED_MOD:
                        log += " mod";
                        break;
                    case TypedBinOp::TYPED_MUL:
                        log += " mul";
                        break;
                    default:
                        log += " other";
                        break;
                }
                break;
            }
            case OpCode::TYPED_UNARY_OP: {
                auto unOp = acc_.GetTypedUnAccessor(gate).GetTypedUnOp();
                switch (unOp) {
                    case TypedUnOp::TYPED_INC:
                        log += " inc";
                        break;
                    case TypedUnOp::TYPED_DEC:
                        log += " dec";
                        break;
                    case TypedUnOp::TYPED_NEG:
                        log += " neg";
                        break;
                    default:
                        log += " other";
                        break;
                }
                break;
            }
            case OpCode::INDEX_CHECK: {
                log += " index check";
                break;
            }
            case OpCode::LOAD_ARRAY_LENGTH: {
                log += " array length";
                break;
            }
            default: {
                log += " other";
                break;
            }
        }
        auto range = GetRange(gate);
        log += " range = [" + std::to_string(range.GetMin()) + "," + std::to_string(range.GetMax()) + "]";
        LOG_COMPILER(INFO) << log;
    }
}
}  // namespace panda::ecmascript::kungfu
