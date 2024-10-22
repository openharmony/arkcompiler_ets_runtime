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

#include "ecmascript/compiler/early_elimination.h"

namespace panda::ecmascript::kungfu {

void EarlyElimination::Initialize()
{
    dependChains_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    renames_.resize(circuit_->GetMaxGateId() + 1, Circuit::NullGate()); // 1: +1 for size
    GateRef entry = acc_.GetDependRoot();
    VisitDependEntry(entry);
}

DependInfoNode* EarlyElimination::GetLoopDependInfo(GateRef depend)
{
    auto depIn = acc_.GetDep(depend);
    auto dependChain = GetDependChain(depIn);
    if (dependChain == nullptr) {
        return nullptr;
    }
    auto newChain = new (chunk_) DependInfoNode(chunk_);
    newChain->CopyFrom(dependChain);
    ChunkSet<GateRef> visited(chunk_);
    ChunkQueue<GateRef> workList(chunk_);
    workList.push(depend);
    visited.insert(acc_.GetDep(depend));
    while (!workList.empty()) {
        auto curDep = workList.front();
        workList.pop();
        if (visited.count(curDep)) {
            continue;
        }
        if (!acc_.IsNotWrite(curDep)) {
            newChain = UpdateWrite(curDep, newChain);
        }
        visited.insert(curDep);
        auto depCount = acc_.GetDependCount(curDep);
        for (size_t i = 0; i < depCount; ++i) {
            workList.push(acc_.GetDep(curDep, i));
        }
    }
    return newChain;
}

GateRef EarlyElimination::VisitDependEntry(GateRef gate)
{
    auto empty = new (chunk_) DependInfoNode(chunk_);
    return UpdateDependChain(gate, empty);
}

GateRef EarlyElimination::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::LOAD_PROPERTY:
        case OpCode::LOAD_ELEMENT:
        case OpCode::LOAD_ARRAY_LENGTH:
        case OpCode::LOAD_TYPED_ARRAY_LENGTH:
        case OpCode::TYPED_ARRAY_CHECK:
        case OpCode::OBJECT_TYPE_CHECK:
        case OpCode::STABLE_ARRAY_CHECK:
        case OpCode::INDEX_CHECK:
        case OpCode::ELEMENTSKIND_CHECK:
        case OpCode::TYPED_CALL_CHECK:
        case OpCode::LOAD_CONST_OFFSET:
        case OpCode::LOAD_HCLASS_FROM_CONSTPOOL:
        case OpCode::TYPED_BINARY_OP:
        case OpCode::TYPED_UNARY_OP:
        case OpCode::JSINLINETARGET_TYPE_CHECK:
        case OpCode::PROTOTYPE_CHECK:
        case OpCode::LOAD_GETTER:
        case OpCode::LOAD_SETTER:
        case OpCode::ECMA_STRING_CHECK:
        case OpCode::BUILTIN_PROTOTYPE_HCLASS_CHECK:
        case OpCode::TYPE_OF_CHECK:
        case OpCode::ARRAY_CONSTRUCTOR_CHECK:
        case OpCode::FLOAT32_ARRAY_CONSTRUCTOR_CHECK:
        case OpCode::OBJECT_CONSTRUCTOR_CHECK:
        case OpCode::BOOLEAN_CONSTRUCTOR_CHECK:
        case OpCode::PROTO_CHANGE_MARKER_CHECK:
        case OpCode::MONO_LOAD_PROPERTY_ON_PROTO:
        case OpCode::LOAD_BUILTIN_OBJECT:
        case OpCode::LOOK_UP_HOLDER:
        case OpCode::IS_CALLABLE_CHECK:
            return TryEliminateGate(gate);
        case OpCode::STATE_SPLIT:
            if (enableFrameStateElimination_) {
                return TryEliminateFrameState(gate);
            }
            break;
        case OpCode::DEPEND_SELECTOR:
            return TryEliminateDependSelector(gate);
        default:
            if (acc_.GetDependCount(gate) == 1) { // 1: depend in is 1
                return TryEliminateOther(gate);
            }
            break;
    }
    return Circuit::NullGate();
}

GateRef EarlyElimination::TryEliminateOther(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) >= 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }

    if (!acc_.IsNotWrite(gate)) {
        dependChain = UpdateWrite(gate, dependChain);
    }

    return UpdateDependChain(gate, dependChain);
}

GateRef EarlyElimination::TryEliminateGate(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    // dependChain is null
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }

    if (!acc_.IsNotWrite(gate)) {
        dependChain = UpdateWrite(gate, dependChain);
        return UpdateDependChain(gate, dependChain);
    }

    auto numIns = acc_.GetNumValueIn(gate);
    for (size_t i = 0; i < numIns; ++i) {
        auto origin = acc_.GetValueIn(gate, i);
        auto checkd = dependChain->LookupCheckedNode(this, origin);
        if (origin != checkd) {
            acc_.ReplaceValueIn(gate, checkd, i);
        }
    }

    // lookup gate, replace
    auto preGate = dependChain->LookupNode(this, gate);
    if (preGate != Circuit::NullGate()) {
        return preGate;
    }
    // update gate, for others elimination
    dependChain = dependChain->UpdateNode(gate);
    return UpdateDependChain(gate, dependChain);
}

GateRef EarlyElimination::TryEliminateFrameState(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::STATE_SPLIT);
    auto depIn = acc_.GetDep(gate);
    auto dependChain = GetDependChain(depIn);
    // dependChain is null
    if (dependChain == nullptr) {
        return Circuit::NullGate();
    }
    // lookup gate, replace
    auto preFrame = dependChain->LookupFrameState();
    auto curFrame = acc_.GetFrameState(gate);
    if ((preFrame != Circuit::NullGate()) && (preFrame != curFrame) &&
        acc_.GetFrameState(preFrame) == acc_.GetFrameState(curFrame)) {
        acc_.UpdateAllUses(curFrame, preFrame);
        auto frameValues = acc_.GetValueIn(curFrame, 1); // 1: frameValues
        acc_.DeleteGate(frameValues);
        acc_.DeleteGate(curFrame);
        return depIn;
    } else {
        dependChain = dependChain->UpdateFrameState(curFrame);
    }
    // update gate, for others elimination

    return UpdateDependChain(gate, dependChain);
}

GateRef EarlyElimination::TryEliminateDependSelector(GateRef gate)
{
    auto state = acc_.GetState(gate);
    if (acc_.IsLoopHead(state)) {
        auto dependChain = GetLoopDependInfo(gate);
        if (dependChain == nullptr) {
            return Circuit::NullGate();
        }
        return UpdateDependChain(gate, dependChain);
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
    DependInfoNode* copy = new (chunk_) DependInfoNode(chunk_);
    copy->CopyFrom(dependChain);
    for (size_t i = 1; i < dependCount; ++i) { // 1: second in
        auto dependIn = acc_.GetDep(gate, i);
        auto tempChain = GetDependChain(dependIn);
        copy->Merge(this, tempChain);
    }
    return UpdateDependChain(gate, copy);
}

GateRef EarlyElimination::UpdateDependChain(GateRef gate, DependInfoNode* dependChain)
{
    ASSERT(dependChain != nullptr);
    auto oldDependChain = GetDependChain(gate);
    if (dependChain->Equals(oldDependChain)) {
        return Circuit::NullGate();
    }
    dependChains_[acc_.GetId(gate)] = dependChain;
    return gate;
}

DependInfoNode* EarlyElimination::UpdateWrite(GateRef gate, DependInfoNode* dependInfo)
{
    if (!enableMemoryAnalysis_) {
        return new (chunk_) DependInfoNode(chunk_);
    }
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::STORE_PROPERTY:
        case OpCode::STORE_PROPERTY_NO_BARRIER:
        case OpCode::STORE_CONST_OFFSET:
        case OpCode::STORE_ELEMENT:
        case OpCode::STORE_MEMORY:
        case OpCode::MIGRATE_ARRAY_WITH_KIND:
        case OpCode::MONO_STORE_PROPERTY_LOOK_UP_PROTO:
        case OpCode::MONO_STORE_PROPERTY:
            return dependInfo->UpdateStoreProperty(this, gate);
        default:
            return new (chunk_) DependInfoNode(chunk_);
    }
}

bool EarlyElimination::MayAccessOneMemory(GateRef lhs, GateRef rhs)
{
    auto rop = acc_.GetOpCode(rhs);
    auto lop = acc_.GetOpCode(lhs);
    switch (rop) {
        case OpCode::STORE_MEMORY:
            ASSERT(acc_.GetMemoryType(rhs) == MemoryType::ELEMENT_TYPE);
            return acc_.GetOpCode(lhs) == OpCode::LOAD_ELEMENT;
        case OpCode::MIGRATE_ARRAY_WITH_KIND: {
            if (lop == OpCode::LOAD_ELEMENT) {
                GateRef lopValueIn = acc_.GetValueIn(lhs, 0); // loadelement receiver
                GateRef ropValueIn = acc_.GetValueIn(rhs, 0); // migrate receiver
                return lopValueIn == ropValueIn;
            }
            return false;
        }
        case OpCode::STORE_ELEMENT: {
            if (lop == OpCode::LOAD_ELEMENT) {
                bool lopIsTypedArray = acc_.TypedOpIsTypedArray(lhs, TypedOpKind::TYPED_LOAD_OP);
                bool ropIsTypedArray = acc_.TypedOpIsTypedArray(rhs, TypedOpKind::TYPED_STORE_OP);
                return lopIsTypedArray == ropIsTypedArray;
            }
            return false;
        }
        case OpCode::STORE_PROPERTY:
        case OpCode::STORE_PROPERTY_NO_BARRIER: {
            if (lop == OpCode::LOAD_PROPERTY) {
                auto loff = acc_.GetValueIn(lhs, 1);
                auto roff = acc_.GetValueIn(rhs, 1);
                ASSERT(acc_.GetOpCode(loff) == OpCode::CONSTANT);
                ASSERT(acc_.GetOpCode(roff) == OpCode::CONSTANT);
                return loff == roff;
            } else if (lop == OpCode::PROTOTYPE_CHECK) {
                auto lindex = acc_.GetHClassIndex(lhs);
                auto rindex = acc_.GetHClassIndex(rhs);
                return (lindex == 0) || (rindex == 0) || (lindex != rindex);
            }
            break;
        }
        case OpCode::STORE_CONST_OFFSET: {
            if (lop == OpCode::LOAD_CONST_OFFSET) {
                auto loff = acc_.GetOffset(lhs);
                auto roff = acc_.GetOffset(rhs);
                return loff == roff;
            }
            break;
        }
        case OpCode::LOAD_PROPERTY:
        case OpCode::MONO_LOAD_PROPERTY_ON_PROTO:
            if (acc_.GetGateType(lhs).Value() != acc_.GetGateType(rhs).Value()) {
                return false;
            }
            break;
        default:
            break;
    }
    return false;
}

bool EarlyElimination::CompareOrder(GateRef lhs, GateRef rhs)
{
    return visitor_->GetGateOrder(lhs) < visitor_->GetGateOrder(rhs);
}

bool EarlyElimination::CheckReplacement(GateRef lhs, GateRef rhs)
{
    if (!acc_.MetaDataEqu(lhs, rhs)) {
        if (acc_.GetOpCode(lhs) != acc_.GetOpCode(rhs)) {
            return false;
        }
    }

    size_t valueCount = acc_.GetNumValueIn(lhs);
    for (size_t i = 0; i < valueCount; i++) {
        if (Rename(acc_.GetValueIn(lhs, i)) != Rename(acc_.GetValueIn(rhs, i))) {
            return false;
        }
    }

    auto opcode = acc_.GetOpCode(lhs);
    switch (opcode) {
        case OpCode::LOAD_ELEMENT: {
            if (acc_.GetTypedLoadOp(lhs) != acc_.GetTypedLoadOp(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::TYPED_BINARY_OP: {
            auto lhsOp = acc_.GetTypedBinaryOp(lhs);
            auto rhsOp = acc_.GetTypedBinaryOp(rhs);
            if (lhsOp != rhsOp) {
                return false;
            }
            break;
        }
        case OpCode::TYPED_UNARY_OP: {
            auto lhsOp = acc_.GetTypedUnAccessor(lhs).GetTypedUnOp();
            auto rhsOp = acc_.GetTypedUnAccessor(rhs).GetTypedUnOp();
            if (lhsOp != rhsOp) {
                return false;
            }
            break;
        }
        case OpCode::TYPED_ARRAY_CHECK: {
            TypedArrayMetaDataAccessor lhsAccessor = acc_.GetTypedArrayMetaDataAccessor(lhs);
            TypedArrayMetaDataAccessor rhsAccessor = acc_.GetTypedArrayMetaDataAccessor(rhs);
            if ((lhsAccessor.GetParamType() != rhsAccessor.GetParamType()) ||
                (lhsAccessor.GetOnHeapMode() != rhsAccessor.GetOnHeapMode())) {
                return false;
            }
            break;
        }
        case OpCode::TYPE_OF_CHECK: {
            if (acc_.GetParamType(lhs) != acc_.GetParamType(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::PROTOTYPE_CHECK: {
            if (acc_.GetHClassIndex(lhs) != acc_.GetHClassIndex(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::LOAD_CONST_OFFSET: {
            if (acc_.GetOffset(lhs) != acc_.GetOffset(rhs)) {
                return false;
            }
            if (acc_.GetMachineType(lhs) != acc_.GetMachineType(rhs)) {
                return false;
            }
            if (acc_.GetMemoryAttribute(lhs).Value() != acc_.GetMemoryAttribute(rhs).Value()) {
                return false;
            }
            break;
        }
        case OpCode::LOAD_HCLASS_FROM_CONSTPOOL: {
            if (acc_.GetIndex(lhs) != acc_.GetIndex(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::JSINLINETARGET_TYPE_CHECK: {
            if (acc_.GetFuncGT(lhs) != acc_.GetFuncGT(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::ARRAY_CONSTRUCTOR_CHECK:
        case OpCode::FLOAT32_ARRAY_CONSTRUCTOR_CHECK:
        case OpCode::OBJECT_CONSTRUCTOR_CHECK:
        case OpCode::BOOLEAN_CONSTRUCTOR_CHECK: {
            if (acc_.GetValueIn(lhs) != acc_.GetValueIn(rhs)) {
                return false;
            }
            break;
        }
        case OpCode::LOAD_BUILTIN_OBJECT: {
            if (acc_.GetIndex(lhs) != acc_.GetIndex(rhs)) {
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool EarlyElimination::CheckRenameReplacement(GateRef lhs, GateRef rhs)
{
    auto opcode = acc_.GetOpCode(lhs);
    switch (opcode) {
        case OpCode::INDEX_CHECK: {
            auto index = acc_.GetValueIn(lhs, 1);
            if (Rename(index) == Rename(rhs)) {
                return true;
            }
            break;
        }
        default:
            break;
    }
    return false;
}

GateRef EarlyElimination::Rename(GateRef gate)
{
    ChunkStack<GateRef> gateStack(chunk_);
    while (true) {
        auto op = acc_.GetOpCode(gate);
        bool renamed = false;
        switch (op) {
            case OpCode::INDEX_CHECK: {
                GateRef ans = renames_[acc_.GetId(gate)];
                if (ans == Circuit::NullGate()) {
                    renamed = true;
                    gateStack.push(gate);
                    gate = acc_.GetValueIn(gate, 1);
                } else {
                    gate = ans;
                }
                break;
            }
            default:
                break;
        }
        if (!renamed) {
            break;
        }
    }
    while (!gateStack.empty()) {
        auto topGate = gateStack.top();
        gateStack.pop();
        renames_[acc_.GetId(topGate)] = gate;
    }
    return gate;
}

void DependInfoNode::Merge(EarlyElimination* elimination, DependInfoNode* that)
{
    auto siz = this->size_; // size of lhs-chain
    auto lhs = this->head_;
    auto rhs = that->head_;
    ChunkStack<GateRef> gateStack(chunk_);
    while (lhs != rhs) {
        if (lhs == nullptr || rhs == nullptr) {
            siz = 0;
            lhs = nullptr;
            break;
        } else if (lhs->gate == rhs->gate) {
            gateStack.push(lhs->gate);
            siz--;
            lhs = lhs->next;
            rhs = rhs->next;
        } else if (elimination->CompareOrder(lhs->gate, rhs->gate)) {
            rhs = rhs->next;
        } else {
            siz--;
            lhs = lhs->next;
        }
    }
    // lhs : common suffix of lhs-chain and rhs-chain
    this->head_ = lhs;
    this->size_ = siz;
    while (!gateStack.empty()) {
        Node* node = chunk_->New<Node>(gateStack.top(), head_);
        gateStack.pop();
        this->size_++;
        this->head_ = node;
    }
    if (this->frameState_ != that->frameState_) {
        this->frameState_ = Circuit::NullGate();
    }
}

bool DependInfoNode::Equals(DependInfoNode* that)
{
    if (that == nullptr) {
        return false;
    }
    if (size_ != that->size_ || frameState_ != that->frameState_) {
        return false;
    }
    auto lhs = this->head_;
    auto rhs = that->head_;
    while (lhs != rhs) {
        if (lhs->gate != rhs->gate) {
            return false;
        }
        lhs = lhs->next;
        rhs = rhs->next;
    }
    return true;
}

GateRef DependInfoNode::LookupFrameState() const
{
    return frameState_;
}

GateRef DependInfoNode::LookupCheckedNode(EarlyElimination* elimination, GateRef gate)
{
    for (Node* node = head_; node != nullptr; node = node->next) {
        if (elimination->CheckRenameReplacement(node->gate, gate)) {
            return node->gate;
        }
    }
    return gate;
}

void DependInfoNode::GetGates(std::vector<GateRef>& gates) const
{
    ChunkStack<GateRef> st(chunk_);
    for (Node* node = head_; node != nullptr; node = node->next) {
        st.push(node->gate);
    }
    while (!st.empty()) {
        gates.emplace_back(st.top());
        st.pop();
    }
}

GateRef DependInfoNode::LookupNode(EarlyElimination* elimination, GateRef gate)
{
    for (Node* node = head_; node != nullptr; node = node->next) {
        if (elimination->CheckReplacement(node->gate, gate)) {
            return node->gate;
        }
    }
    return Circuit::NullGate();
}

DependInfoNode* DependInfoNode::UpdateNode(GateRef gate)
{
    // assign node->next to head
    Node* node = chunk_->New<Node>(gate, head_);
    DependInfoNode* that = new (chunk_) DependInfoNode(chunk_);
    // assign head to node
    that->head_ = node;
    that->size_ = size_ + 1;
    that->frameState_ = frameState_;
    return that;
}

DependInfoNode* DependInfoNode::UpdateFrameState(GateRef framestate)
{
    // assign node->next to head
    DependInfoNode* that = new (chunk_) DependInfoNode(chunk_);
    // assign head to node
    that->head_ = head_;
    that->size_ = size_;
    that->frameState_ = framestate;
    return that;
}

DependInfoNode* DependInfoNode::UpdateStoreProperty(EarlyElimination* elimination, GateRef gate)
{
    DependInfoNode* that = new (chunk_) DependInfoNode(chunk_);
    ChunkStack<GateRef> gateStack(chunk_);
    for (Node* node = head_; node != nullptr; node = node->next) {
        if (!elimination->MayAccessOneMemory(node->gate, gate)) {
            gateStack.push(node->gate);
        }
    }
    while (!gateStack.empty()) {
        that = that->UpdateNode(gateStack.top());
        gateStack.pop();
    }
    return that;
}
}  // namespace panda::ecmascript::kungfu
