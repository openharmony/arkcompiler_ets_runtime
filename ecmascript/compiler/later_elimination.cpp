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
#include "ecmascript/compiler/later_elimination.h"

namespace panda::ecmascript::kungfu {

void LaterElimination::Initialize()
{
    dependChains_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    GateRef entry = acc_.GetDependRoot();
    VisitDependEntry(entry);
}

GateRef LaterElimination::VisitDependEntry(GateRef gate)
{
    auto empty = new (chunk_) DependChains(chunk_);
    return UpdateDependChain(gate, empty);
}

GateRef LaterElimination::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::GET_ENV:
        case OpCode::GET_GLOBAL_ENV:
        case OpCode::GET_GLOBAL_ENV_OBJ:
        case OpCode::GET_GLOBAL_ENV_OBJ_HCLASS:
        case OpCode::GET_GLOBAL_CONSTANT_VALUE:
        case OpCode::ARRAY_GUARDIAN_CHECK:
        case OpCode::HCLASS_STABLE_ARRAY_CHECK:
        case OpCode::HEAP_OBJECT_CHECK:
        case OpCode::ECMA_OBJECT_CHECK:
        case OpCode::INT32_UNSIGNED_UPPER_BOUND_CHECK:
        case OpCode::OVERFLOW_CHECK:
        case OpCode::VALUE_CHECK_NEG_OVERFLOW:
        case OpCode::FLOAT64_CHECK_RIGHT_IS_ZERO:
        case OpCode::INT32_CHECK_RIGHT_IS_ZERO:
        case OpCode::INT32_DIV_WITH_CHECK:
        case OpCode::LEX_VAR_IS_HOLE_CHECK:
        case OpCode::COW_ARRAY_CHECK:
        case OpCode::FLATTEN_TREE_STRING_CHECK:
        case OpCode::CHECK_AND_CONVERT:
        case OpCode::TAGGED_IS_HEAP_OBJECT:
        case OpCode::IS_MARKER_CELL_VALID:
        case OpCode::IS_SPECIFIC_OBJECT_TYPE:
            return TryEliminateGate(gate);
        case OpCode::DEPEND_SELECTOR:
            return TryEliminateDependSelector(gate);
        default:
            if (acc_.GetDependCount(gate) == 1) { // 1: depend in is 1
                return TryEliminateOther(gate);
            }
    }
    return Circuit::NullGate();
}

GateRef LaterElimination::TryEliminateOther(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) >= 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    return UpdateDependChain(gate, dependChain);
}

GateRef LaterElimination::TryEliminateGate(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    // dependChain is null
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    // lookup gate, replace
    auto preGate = LookupNode(dependChain, gate);
    if (preGate != Circuit::NullGate()) {
        return preGate;
    }
    // update gate, for others elimination
    dependChain = dependChain->UpdateNode(gate);
    return UpdateDependChain(gate, dependChain);
}

GateRef LaterElimination::TryEliminateDependSelector(GateRef gate)
{
    auto state = acc_.GetState(gate);
    if (acc_.IsLoopHead(state)) {
        // use loop head as depend chain
        return TryEliminateOther(gate);
    }

    auto dependCount = acc_.GetDependCount(gate);
    for (size_t i = 0; i < dependCount; ++i) {
        auto depend = acc_.GetDep(gate, i);
        auto dependChain = GetDependChain(depend);
        if (dependChain == nullptr) {
            return Circuit::NullGate();
        }
    }

    // all depend done.
    auto depend = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depend);
    DependChains* copy = new (chunk_) DependChains(chunk_);
    copy->CopyFrom(dependChain);
    for (size_t i = 1; i < dependCount; ++i) { // 1: second in
        auto dependIn = acc_.GetDep(gate, i);
        auto tempChain = GetDependChain(dependIn);
        copy->Merge(tempChain);
    }
    return UpdateDependChain(gate, copy);
}

GateRef LaterElimination::UpdateDependChain(GateRef gate, DependChains* dependChain)
{
    ASSERT(dependChain != nullptr);
    auto oldDependChain = GetDependChain(gate);
    if (dependChain->Equals(oldDependChain)) {
        return Circuit::NullGate();
    }
    dependChains_[acc_.GetId(gate)] = dependChain;
    return gate;
}

bool LaterElimination::CheckReplacement(GateRef lhs, GateRef rhs)
{
    if (!acc_.MetaDataEqu(lhs, rhs)) {
        if (acc_.GetOpCode(lhs) != acc_.GetOpCode(rhs)) {
            return false;
        }
    }
    size_t valueCount = acc_.GetNumValueIn(lhs);
    for (size_t i = 0; i < valueCount; i++) {
        if (acc_.GetValueIn(lhs, i) != acc_.GetValueIn(rhs, i)) {
            return false;
        }
    }
    auto opcode = acc_.GetOpCode(lhs);
    switch (opcode) {
        case OpCode::GET_GLOBAL_ENV_OBJ:
        case OpCode::GET_GLOBAL_ENV_OBJ_HCLASS:
        case OpCode::GET_GLOBAL_CONSTANT_VALUE: {
            if (acc_.GetIndex(lhs) != acc_.GetIndex(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::IS_SPECIFIC_OBJECT_TYPE: {
            if (acc_.GetJSType(lhs) != acc_.GetJSType(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::CHECK_AND_CONVERT: {
            if (acc_.GetSrcType(lhs) != acc_.GetSrcType(rhs)) {
                return false;
            }
            if (acc_.GetDstType(lhs) != acc_.GetDstType(rhs)) {
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}
}  // namespace panda::ecmascript::kungfu
