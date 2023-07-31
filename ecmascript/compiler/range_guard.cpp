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
#include "ecmascript/compiler/range_guard.h"

namespace panda::ecmascript::kungfu {
void RangeGuard::Run()
{
    dependChains_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    GateRef entry = acc_.GetDependRoot();
    VisitDependEntry(entry);
    VisitGraph();
}

GateRef RangeGuard::VisitGate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::VALUE_SELECTOR:
        case OpCode::TYPED_BINARY_OP:
        case OpCode::TYPED_UNARY_OP:
        case OpCode::INDEX_CHECK: {
            return TryApplyRangeGuardGate(gate);
        }
        case OpCode::DEPEND_SELECTOR: {
            return TraverseDependSelector(gate);
        }
        default: {
            if (acc_.GetDependCount(gate) == 1) { // 1: depend in is 1
                return TraverseOthers(gate);
            }
            break;
        }
    }
    return Circuit::NullGate();
}

GateRef RangeGuard::TraverseOthers(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) >= 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }

    return UpdateDependChain(gate, dependChain);
}

GateRef RangeGuard::TraverseDependSelector(GateRef gate)
{
    auto state = acc_.GetState(gate);
    if (acc_.IsLoopHead(state)) {
        return TraverseOthers(gate);
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

GateRef RangeGuard::TryApplyTypedArrayRangeGuardForLength(DependChains* dependChain, GateRef gate, GateRef input)
{
    ASSERT(dependChain != nullptr);
    if (dependChain->FoundIndexCheckedForLength(this, input)) {
        Environment env(gate, circuit_, &builder_);
        auto rangeGuardGate = builder_.RangeGuard(input, 1, RangeInfo::TYPED_ARRAY_ONHEAP_MAX); // If the IndexCheck before the TypeArrayLength used, the TypeArrayLength must start by 1.
        return rangeGuardGate;
    }
    return Circuit::NullGate();
}

GateRef RangeGuard::TryApplyTypedArrayRangeGuardForIndex(DependChains* dependChain, GateRef gate, GateRef input)
{
    ASSERT(dependChain != nullptr);
    if (dependChain->FoundIndexCheckedForIndex(this, input)) {
        Environment env(gate, circuit_, &builder_);
        auto rangeGuardGate = builder_.RangeGuard(input, 0, RangeInfo::TYPED_ARRAY_ONHEAP_MAX); // If the IndexCheck used in the TypeArray, the index must in the TypeArray range.
        return rangeGuardGate;
    }
    return Circuit::NullGate();
}

GateRef RangeGuard::TryApplyRangeGuardGate(GateRef gate)
{
    if (acc_.GetDependCount(gate) < 1) {
        return Circuit::NullGate();
    }

    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    // dependChain is null
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }

    auto numIns = acc_.GetInValueCount(gate);
    for (size_t i = 0; i < numIns; ++i) {
        auto originalInput = acc_.GetValueIn(gate, i);
        auto originalInputOpcode = acc_.GetOpCode(originalInput);
        auto rangeGuardGate = Circuit::NullGate();
        if (originalInputOpcode == OpCode::LOAD_TYPED_ARRAY_LENGTH) {
            rangeGuardGate = TryApplyTypedArrayRangeGuardForLength(dependChain, gate, originalInput);
        } else if(originalInputOpcode != OpCode::CONSTANT) {
            rangeGuardGate = TryApplyTypedArrayRangeGuardForIndex(dependChain, gate, originalInput);
        }
        if (rangeGuardGate != Circuit::NullGate()) {
            acc_.ReplaceValueIn(gate, rangeGuardGate, i);
        }
    }
    dependChain = dependChain->UpdateNode(gate);
    return UpdateDependChain(gate, dependChain);
}

GateRef RangeGuard::VisitDependEntry(GateRef gate)
{
    auto empty = new (chunk_) DependChains(chunk_);
    return UpdateDependChain(gate, empty);
}

GateRef RangeGuard::UpdateDependChain(GateRef gate, DependChains* dependChain)
{
    ASSERT(dependChain != nullptr);
    auto oldDependChain = GetDependChain(gate);
    if (dependChain->Equals(oldDependChain)) {
        return Circuit::NullGate();
    }
    dependChains_[acc_.GetId(gate)] = dependChain;
    return gate;
}

bool RangeGuard::CheckIndexCheckLengthInput(GateRef lhs, GateRef rhs)
{
    auto lhsOpcode = acc_.GetOpCode(lhs);
    if (lhsOpcode == OpCode::INDEX_CHECK) {
        auto indexCheckLengthInput = acc_.GetValueIn(lhs, 0); // length
        return indexCheckLengthInput == rhs;
    }
    return false;
}

bool RangeGuard::CheckIndexCheckIndexInput(GateRef lhs, GateRef rhs)
{
    auto lhsOpcode = acc_.GetOpCode(lhs);
    if (lhsOpcode == OpCode::INDEX_CHECK) {
        auto indexCheckLengthInput = acc_.GetValueIn(lhs, 0); // length
        auto indexCheckIndexInput = acc_.GetValueIn(lhs, 1); // index
        auto indexCheckLengthInputOpcode = acc_.GetOpCode(indexCheckLengthInput);
        if(indexCheckIndexInput == rhs && indexCheckLengthInputOpcode == OpCode::LOAD_TYPED_ARRAY_LENGTH) { // TYPED_ARRAY 
            return RangeInfo::TYPED_ARRAY_ONHEAP_MAX;
        }
    }
    return false;
}
}  // namespace panda::ecmascript::kungfu
