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

#include <queue>
#include <stack>

#include "ecmascript/compiler/early_elimination.h"

namespace panda::ecmascript::kungfu {

GateRef DependChainInfo::LookUpElement(ElementInfo* info) const
{
    if ((elementMap_ != nullptr) && (elementMap_->count(*info) > 0)) {
        return elementMap_->at(*info);
    } else {
        return Circuit::NullGate();
    }
}

GateRef DependChainInfo::LookUpProperty(PropertyInfo* info) const
{
    if ((propertyMap_ != nullptr) && (propertyMap_->count(*info) > 0)) {
        return propertyMap_->at(*info);
    } else {
        return Circuit::NullGate();
    }
}

GateRef DependChainInfo::LookUpArrayLength(ArrayLengthInfo* info) const
{
    if ((arrayLengthMap_ != nullptr) && (arrayLengthMap_->count(*info) > 0)) {
        return arrayLengthMap_->at(*info);
    } else {
        return Circuit::NullGate();
    }
}

bool DependChainInfo::LookUpPrimitiveTypeCheck(PrimitiveTypeCheckInfo* info) const
{
    return (primitiveTypeCheckSet_ != nullptr) && (primitiveTypeCheckSet_->count(*info) > 0);
}

bool DependChainInfo::LookUpInt32OverflowCheck(Int32OverflowCheckInfo* info) const
{
    return (int32OverflowCheckSet_ != nullptr) && (int32OverflowCheckSet_->count(*info) > 0);
}

bool DependChainInfo::LookUpArrayCheck(ArrayCheckInfo* info) const
{
    return (arrayCheckSet_ != nullptr) && (arrayCheckSet_->count(*info) > 0);
}

bool DependChainInfo::LookUpStableArrayCheck(StableArrayCheckInfo* info) const
{
    return (stableArrayCheckSet_ != nullptr) && (stableArrayCheckSet_->count(*info) > 0);
}

bool DependChainInfo::LookUpTypedArrayCheck(TypedArrayCheckInfo* info) const
{
    return (typedArrayCheckSet_ != nullptr) && (typedArrayCheckSet_->count(*info) > 0);
}

bool DependChainInfo::LookUpObjectTypeCheck(ObjectTypeCheckInfo* info) const
{
    return (objectTypeCheckSet_ != nullptr) && (objectTypeCheckSet_->count(*info) > 0);
}

bool DependChainInfo::LookUpIndexCheck(IndexCheckInfo* info) const
{
    return (indexCheckSet_ != nullptr) && (indexCheckSet_->count(*info) > 0);
}

bool DependChainInfo::LookUpTypedCallCheck(TypedCallCheckInfo* info) const
{
    return (typedCallCheckSet_ != nullptr) && (typedCallCheckSet_->count(*info) > 0);
}

GateRef DependChainInfo::LookUpFrameState() const
{
    return frameState_;
}

DependChainInfo* DependChainInfo::UpdateProperty(PropertyInfo* info, GateRef gate)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (propertyMap_ != nullptr) {
        that->propertyMap_ = new ChunkMap<PropertyInfo, GateRef>(*propertyMap_);
    } else {
        that->propertyMap_ = new ChunkMap<PropertyInfo, GateRef>(chunk_);
    }
    that->propertyMap_->insert(std::make_pair(*info, gate));
    return that;
}

DependChainInfo* DependChainInfo::UpdateElement(ElementInfo* info, GateRef gate)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (elementMap_ != nullptr) {
        that->elementMap_ = new ChunkMap<ElementInfo, GateRef>(*elementMap_);
    } else {
        that->elementMap_ = new ChunkMap<ElementInfo, GateRef>(chunk_);
    }
    that->elementMap_->insert(std::make_pair(*info, gate));
    return that;
}

DependChainInfo* DependChainInfo::UpdateArrayLength(ArrayLengthInfo* info, GateRef gate)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (arrayLengthMap_ != nullptr) {
        that->arrayLengthMap_ = new ChunkMap<ArrayLengthInfo, GateRef>(*arrayLengthMap_);
    } else {
        that->arrayLengthMap_ = new ChunkMap<ArrayLengthInfo, GateRef>(chunk_);
    }
    that->arrayLengthMap_->insert(std::make_pair(*info, gate));
    return that;
}

DependChainInfo* DependChainInfo::UpdatePrimitiveTypeCheck(PrimitiveTypeCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (primitiveTypeCheckSet_ != nullptr) {
        that->primitiveTypeCheckSet_ = new ChunkSet<PrimitiveTypeCheckInfo>(*primitiveTypeCheckSet_);
    } else {
        that->primitiveTypeCheckSet_ = new ChunkSet<PrimitiveTypeCheckInfo>(chunk_);
    }
    that->primitiveTypeCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateInt32OverflowCheck(Int32OverflowCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (int32OverflowCheckSet_ != nullptr) {
        that->int32OverflowCheckSet_ = new ChunkSet<Int32OverflowCheckInfo>(*int32OverflowCheckSet_);
    } else {
        that->int32OverflowCheckSet_ = new ChunkSet<Int32OverflowCheckInfo>(chunk_);
    }
    that->int32OverflowCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateArrayCheck(ArrayCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (arrayCheckSet_ != nullptr) {
        that->arrayCheckSet_ = new ChunkSet<ArrayCheckInfo>(*arrayCheckSet_);
    } else {
        that->arrayCheckSet_ = new ChunkSet<ArrayCheckInfo>(chunk_);
    }
    that->arrayCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateStableArrayCheck(StableArrayCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (stableArrayCheckSet_ != nullptr) {
        that->stableArrayCheckSet_ = new ChunkSet<StableArrayCheckInfo>(*stableArrayCheckSet_);
    } else {
        that->stableArrayCheckSet_ = new ChunkSet<StableArrayCheckInfo>(chunk_);
    }
    that->stableArrayCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateTypedArrayCheck(TypedArrayCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (typedArrayCheckSet_ != nullptr) {
        that->typedArrayCheckSet_ = new ChunkSet<TypedArrayCheckInfo>(*typedArrayCheckSet_);
    } else {
        that->typedArrayCheckSet_ = new ChunkSet<TypedArrayCheckInfo>(chunk_);
    }
    that->typedArrayCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateObjectTypeCheck(ObjectTypeCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (objectTypeCheckSet_ != nullptr) {
        that->objectTypeCheckSet_ = new ChunkSet<ObjectTypeCheckInfo>(*objectTypeCheckSet_);
    } else {
        that->objectTypeCheckSet_ = new ChunkSet<ObjectTypeCheckInfo>(chunk_);
    }
    that->objectTypeCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateIndexCheck(IndexCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (indexCheckSet_ != nullptr) {
        that->indexCheckSet_ = new ChunkSet<IndexCheckInfo>(*indexCheckSet_);
    } else {
        that->indexCheckSet_ = new ChunkSet<IndexCheckInfo>(chunk_);
    }
    that->indexCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateTypedCallCheck(TypedCallCheckInfo* info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (typedCallCheckSet_ != nullptr) {
        that->typedCallCheckSet_ = new ChunkSet<TypedCallCheckInfo>(*typedCallCheckSet_);
    } else {
        that->typedCallCheckSet_ = new ChunkSet<TypedCallCheckInfo>(chunk_);
    }
    that->typedCallCheckSet_->insert(*info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateFrameState(GateRef gate)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    that->frameState_ = gate;
    return that;
}

DependChainInfo* DependChainInfo::UpdateWrite()
{
    // save primitiveTypeCheckSet_ and int32OverflowCheckSet_ since these checks have no side effect
    DependChainInfo* that = new (chunk_) DependChainInfo(chunk_);
    that->primitiveTypeCheckSet_ = primitiveTypeCheckSet_;
    that->int32OverflowCheckSet_ = int32OverflowCheckSet_;
    return that;
}

bool DependChainInfo::Empty() const
{
    return (elementMap_ == nullptr) &&
           (propertyMap_ == nullptr) &&
           (arrayLengthMap_ == nullptr) &&
           (primitiveTypeCheckSet_ == nullptr) &&
           (int32OverflowCheckSet_ == nullptr) &&
           (arrayCheckSet_ == nullptr) &&
           (stableArrayCheckSet_ == nullptr) &&
           (typedArrayCheckSet_ == nullptr) &&
           (objectTypeCheckSet_ == nullptr) &&
           (indexCheckSet_ == nullptr) &&
           (typedCallCheckSet_ == nullptr) &&
           (frameState_ == Circuit::NullGate());
}

bool DependChainInfo::Equal(DependChainInfo* that)
{
    if (this == that) return true;
    if (that == nullptr) return false;
    return *this == *that;
}

template<typename K, typename V>
ChunkMap<K, V>* DependChainInfo::MergeMap(ChunkMap<K, V>* thisMap, ChunkMap<K, V>* thatMap)
{
    if (thisMap == thatMap) {
        return thisMap;
    } else if (thisMap == nullptr || thatMap == nullptr) {
        return nullptr;
    } else {
        auto newMap = new ChunkMap<K, V>(chunk_);
        const auto &tempMap = *thisMap;
        for (const auto &pr : tempMap) {
            if (thatMap->count(pr.first) && thatMap->at(pr.first) == pr.second) {
                newMap->insert(pr);
            }
        }
        return newMap;
    }
}

template<typename K>
ChunkSet<K>* DependChainInfo::MergeSet(ChunkSet<K>* thisSet, ChunkSet<K>* thatSet)
{
    if (thisSet == thatSet) {
        return thisSet;
    } else if (thisSet == nullptr || thatSet == nullptr) {
        return nullptr;
    } else {
        auto newSet = new ChunkSet<K>(chunk_);
        const auto &tempSet = *thisSet;
        for (const auto &it : tempSet) {
            if (thatSet->count(it)) {
                newSet->insert(it);
            }
        }
        return newSet;
    }
}

DependChainInfo* DependChainInfo::Merge(DependChainInfo* that)
{
    if (Equal(that)) {
        return that;
    }
    DependChainInfo* newInfo = new (chunk_) DependChainInfo(*this);
    newInfo->elementMap_ = MergeMap<ElementInfo, GateRef>(elementMap_, that->elementMap_);
    newInfo->propertyMap_ = MergeMap<PropertyInfo, GateRef>(propertyMap_, that->propertyMap_);
    newInfo->arrayLengthMap_ = MergeMap<ArrayLengthInfo, GateRef>(arrayLengthMap_, that->arrayLengthMap_);
    newInfo->primitiveTypeCheckSet_ =
        MergeSet<PrimitiveTypeCheckInfo>(primitiveTypeCheckSet_, that->primitiveTypeCheckSet_);
    newInfo->int32OverflowCheckSet_ =
        MergeSet<Int32OverflowCheckInfo>(int32OverflowCheckSet_, that->int32OverflowCheckSet_);
    newInfo->arrayCheckSet_ = MergeSet<ArrayCheckInfo>(arrayCheckSet_, that->arrayCheckSet_);
    newInfo->stableArrayCheckSet_ = MergeSet<StableArrayCheckInfo>(stableArrayCheckSet_, that->stableArrayCheckSet_);
    newInfo->typedArrayCheckSet_ = MergeSet<TypedArrayCheckInfo>(typedArrayCheckSet_, that->typedArrayCheckSet_);
    newInfo->objectTypeCheckSet_ = MergeSet<ObjectTypeCheckInfo>(objectTypeCheckSet_, that->objectTypeCheckSet_);
    newInfo->indexCheckSet_ = MergeSet<IndexCheckInfo>(indexCheckSet_, that->indexCheckSet_);
    newInfo->typedCallCheckSet_ = MergeSet<TypedCallCheckInfo>(typedCallCheckSet_, that->typedCallCheckSet_);
    newInfo->frameState_ = frameState_ == that->frameState_ ? frameState_ : Circuit::NullGate();
    return newInfo;
}

void EarlyElimination::Run()
{
    RemoveRedundantGate();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After check eliminating "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}

bool EarlyElimination::IsSideEffectLoop(GateRef depend)
{
    ChunkSet<GateRef> visited(GetChunk());
    ChunkQueue<GateRef> workList(GetChunk());
    workList.push(depend);
    visited.insert(acc_.GetDep(depend));
    while (!workList.empty()) {
        auto curDep = workList.front();
        workList.pop();
        if (visited.count(curDep)) {
            continue;
        }
        if (!acc_.IsNotWrite(curDep)) {
            return true;
        }
        visited.insert(curDep);
        auto depCount = acc_.GetDependCount(curDep);
        for (size_t i = 0; i < depCount; ++i) {
            workList.push(acc_.GetDep(curDep, i));
        }
    }
    return false;
}

ElementInfo* EarlyElimination::GetElementInfo(GateRef gate) const
{
    auto op = acc_.GetTypedLoadOp(gate);
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    return new (GetChunk()) ElementInfo(op, v0, v1);
}

PropertyInfo* EarlyElimination::GetPropertyInfo(GateRef gate) const
{
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    return new (GetChunk()) PropertyInfo(v0, v1);
}

ArrayLengthInfo* EarlyElimination::GetArrayLengthInfo(GateRef gate) const
{
    auto v0 = acc_.GetValueIn(gate, 0);
    return new (GetChunk()) ArrayLengthInfo(v0);
}

PrimitiveTypeCheckInfo* EarlyElimination::GetPrimitiveTypeCheckInfo(GateRef gate) const
{
    auto type = acc_.GetParamGateType(gate);
    auto v0 = acc_.GetValueIn(gate, 0);
    return new (GetChunk()) PrimitiveTypeCheckInfo(type, v0);
}

Int32OverflowCheckInfo* EarlyElimination::GetInt32OverflowCheckInfo(GateRef gate) const
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    auto op = accessor.GetTypedUnOp();
    auto v0 = acc_.GetValueIn(gate, 0);
    return new (GetChunk()) Int32OverflowCheckInfo(op, v0);
}

ArrayCheckInfo* EarlyElimination::GetArrayCheckInfo(GateRef gate) const
{
    auto v0 = acc_.GetValueIn(gate, 0);
    return new (GetChunk()) ArrayCheckInfo(v0);
}

StableArrayCheckInfo* EarlyElimination::GetStableArrayCheckInfo(GateRef gate) const
{
    auto v0 =  acc_.GetValueIn(gate, 0);
    return new (GetChunk()) StableArrayCheckInfo(v0);
}

TypedArrayCheckInfo* EarlyElimination::GetTypedArrayCheckInfo(GateRef gate) const
{
    auto type = acc_.GetParamGateType(gate);
    auto v0 = acc_.GetValueIn(gate, 0);
    return new (GetChunk()) TypedArrayCheckInfo(type, v0);
}

ObjectTypeCheckInfo* EarlyElimination::GetObjectTypeCheckInfo(GateRef gate) const
{
    auto type = acc_.GetParamGateType(gate);
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    return new (GetChunk()) ObjectTypeCheckInfo(type, v0, v1);
}

IndexCheckInfo* EarlyElimination::GetIndexCheckInfo(GateRef gate) const
{
    auto type = acc_.GetParamGateType(gate);
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    return new (GetChunk()) IndexCheckInfo(type, v0, v1);
}

TypedCallCheckInfo* EarlyElimination::GetTypedCallCheckInfo(GateRef gate) const
{
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    auto v2 = acc_.GetValueIn(gate, 2);
    return new (GetChunk()) TypedCallCheckInfo(v0, v1, v2);
}

bool EarlyElimination::IsPrimitiveTypeCheck(GateRef gate) const
{
    auto op = acc_.GetOpCode(gate);
    return op == OpCode::PRIMITIVE_TYPE_CHECK;
}

bool EarlyElimination::IsTrustedType(GateRef gate) const
{
    if (acc_.IsConstant(gate)) {
        return true;
    }
    if (acc_.IsTypedOperator(gate)) {
        if (acc_.GetOpCode(gate) == OpCode::TYPED_BINARY_OP) {
            return !acc_.GetGateType(gate).IsIntType();
        } else {
            return true;
        }
    }
    return false;
}

void EarlyElimination::TrustedTypePropagate(ChunkQueue<GateRef>& workList, const ChunkVector<GateRef>& checkList)
{
    ChunkUnorderedMap<GateRef, size_t> trustedInCount(GetChunk());
    while (!workList.empty()) {
        auto gate = workList.front();
        workList.pop();
        auto uses = acc_.Uses(gate);
        for (auto i = uses.begin(); i != uses.end(); i++) {
            GateRef phi = *i;
            if ((acc_.GetOpCode(phi) != OpCode::VALUE_SELECTOR) ||
                (acc_.GetGateType(phi) != acc_.GetGateType(gate))) {
                continue;
            }
            trustedInCount[phi]++;
            if (trustedInCount.at(phi) == acc_.GetNumValueIn(phi)) {
                workList.push(phi);
            }
        }
    }
    for (auto check : checkList) {
        ASSERT(acc_.GetOpCode(check) == OpCode::PRIMITIVE_TYPE_CHECK);
        auto value = acc_.GetValueIn(check, 0);
        ASSERT(acc_.GetGateType(value) == acc_.GetParamGateType(check));
        if (IsTrustedType(value)) {
            RemoveGate(check, Circuit::NullGate());
            continue;
        }
        if ((trustedInCount.count(value) != 0) &&
            (trustedInCount.at(value) == acc_.GetNumValueIn(value))) {
            RemoveGate(check, Circuit::NullGate());
            continue;
        }
        // remove check
    }
}

void EarlyElimination::TryEliminate(GateRef gate)
{
    auto op = acc_.GetOpCode(gate);
    switch (op) {
        case OpCode::LOAD_PROPERTY:
            TryEliminateProperty(gate);
            break;
        case OpCode::LOAD_ELEMENT:
            TryEliminateElement(gate);
            break;
        case OpCode::LOAD_ARRAY_LENGTH:
            TryEliminateArrayLength(gate);
            break;
        case OpCode::PRIMITIVE_TYPE_CHECK:
            TryEliminatePrimitiveTypeCheck(gate);
            break;
        case OpCode::INT32_OVERFLOW_CHECK:
            TryEliminateInt32OverflowCheck(gate);
            break;
        case OpCode::ARRAY_CHECK:
            TryEliminateArrayCheck(gate);
            break;
        case OpCode::STABLE_ARRAY_CHECK:
            TryEliminateStableArrayCheck(gate);
            break;
        case OpCode::TYPED_ARRAY_CHECK:
            TryEliminateTypedArrayCheck(gate);
            break;
        case OpCode::OBJECT_TYPE_CHECK:
            TryEliminateObjectTypeCheck(gate);
            break;
        case OpCode::INDEX_CHECK:
            TryEliminateIndexCheck(gate);
            break;
        case OpCode::TYPED_CALL_CHECK:
            TryEliminateTypedCallCheck(gate);
            break;
        case OpCode::STATE_SPLIT:
            TryEliminateStateSplitAndFrameState(gate);
            break;
        case OpCode::DEPEND_SELECTOR:
            TryEliminateDependSelector(gate);
            break;
        case OpCode::DEPEND_AND:
            TryEliminateDependAnd(gate);
            break;
        case OpCode::DEPEND_ENTRY:
            return;
        default:
            TryEliminateOther(gate);
            break;
    }
}

void EarlyElimination::TryEliminateElement(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetElementInfo(gate);
    auto preGate = dependInfo->LookUpElement(info);
    if (preGate != Circuit::NullGate()) {
        RemoveGate(gate, preGate);
    } else {
        dependInfo = dependInfo->UpdateElement(info, gate);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateProperty(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetPropertyInfo(gate);
    auto preGate = dependInfo->LookUpProperty(info);
    if (preGate != Circuit::NullGate()) {
        RemoveGate(gate, preGate);
    } else {
        dependInfo = dependInfo->UpdateProperty(info, gate);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateArrayLength(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetArrayLengthInfo(gate);
    auto preGate = dependInfo->LookUpArrayLength(info);
    if (preGate != Circuit::NullGate()) {
        RemoveGate(gate, preGate);
    } else {
        dependInfo = dependInfo->UpdateArrayLength(info, gate);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminatePrimitiveTypeCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetPrimitiveTypeCheckInfo(gate);
    if (dependInfo->LookUpPrimitiveTypeCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdatePrimitiveTypeCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateInt32OverflowCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetInt32OverflowCheckInfo(gate);
    if (dependInfo->LookUpInt32OverflowCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdateInt32OverflowCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateArrayCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetArrayCheckInfo(gate);
    if (dependInfo->LookUpArrayCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdateArrayCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateStableArrayCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetStableArrayCheckInfo(gate);
    if (dependInfo->LookUpStableArrayCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdateStableArrayCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateTypedArrayCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetTypedArrayCheckInfo(gate);
    if (dependInfo->LookUpTypedArrayCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdateTypedArrayCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateObjectTypeCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetObjectTypeCheckInfo(gate);
    if (dependInfo->LookUpObjectTypeCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdateObjectTypeCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateIndexCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetIndexCheckInfo(gate);
    if (dependInfo->LookUpIndexCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdateIndexCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateTypedCallCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto info = GetTypedCallCheckInfo(gate);
    if (dependInfo->LookUpTypedCallCheck(info)) {
        RemoveGate(gate, Circuit::NullGate());
    } else {
        dependInfo = dependInfo->UpdateTypedCallCheck(info);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
    }
}

void EarlyElimination::TryEliminateStateSplitAndFrameState(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    auto frameState = dependInfo->LookUpFrameState();
    auto curFrameState = acc_.GetFrameState(gate);
    if (frameState != Circuit::NullGate()) {
        acc_.UpdateAllUses(curFrameState, frameState);
        RemoveGate(gate, Circuit::NullGate());
        acc_.DeleteGate(curFrameState);
    } else {
        dependInfo = dependInfo->UpdateFrameState(curFrameState);
        dependInfos_[acc_.GetId(gate)] = dependInfo;
        stateSplits_.emplace_back(gate);
    }
    return ;
}

void EarlyElimination::TryEliminateOther(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (!acc_.IsNotWrite(gate)) {
        dependInfo = dependInfo->UpdateWrite();
    }
    dependInfos_[acc_.GetId(gate)] = dependInfo;
    return ;
}

void EarlyElimination::TryEliminateDependSelector(GateRef gate)
{
    auto state = acc_.GetState(gate);
    DependChainInfo* dependInfo = nullptr;
    if (acc_.IsLoopHead(state)) {
        auto depIn = acc_.GetDep(gate);
        dependInfo = dependInfos_[acc_.GetId(depIn)];
        if (IsSideEffectLoop(gate)) {
            dependInfo = dependInfo->UpdateWrite();
        }
    } else {
        auto dependCount = acc_.GetDependCount(gate);
        for (size_t i = 0; i < dependCount; ++i) {
            auto depIn = acc_.GetDep(gate, i);
            auto tempInfo = dependInfos_[acc_.GetId(depIn)];
            if (dependInfo == nullptr) {
                dependInfo = tempInfo;
            } else {
                dependInfo = dependInfo->Merge(tempInfo);
            }
        }
    }
    dependInfos_[acc_.GetId(gate)] = dependInfo;
}

void EarlyElimination::TryEliminateDependAnd(GateRef gate)
{
    auto dep0 = acc_.GetDep(gate, 0);
    auto info0 = dependInfos_[acc_.GetId(dep0)];
    auto dep1 = acc_.GetDep(gate, 1);
    auto info1 = dependInfos_[acc_.GetId(dep1)];
    ASSERT(info0->Empty() || info1->Empty());
    dependInfos_[acc_.GetId(gate)] = (!info0->Empty()) ? info0 : info1;
}

void EarlyElimination::RemoveGate(GateRef gate, GateRef value)
{
    auto state = acc_.GetStateCount(gate) > 0 ? acc_.GetState(gate) : Circuit::NullGate();
    auto depend = acc_.GetDependCount(gate) > 0 ? acc_.GetDep(gate) : Circuit::NullGate();
    auto uses = acc_.Uses(gate);
    for (auto i = uses.begin(); i != uses.end();) {
        if (acc_.IsStateIn(i)) {
            i = acc_.ReplaceIn(i, state);
        } else if (acc_.IsDependIn(i)) {
            i = acc_.ReplaceIn(i, depend);
        } else {
            i = acc_.ReplaceIn(i, value);
        }
    }
    acc_.DeleteGate(gate);
}

void EarlyElimination::RemoveRedundantGate()
{
    RemoveTypeTrustedCheck();
    auto emptyInfo = new (GetChunk()) DependChainInfo(GetChunk());
    dependInfos_.resize(maxId_ + 1, emptyInfo);
    ChunkQueue<GateRef> workList(GetChunk());
    workList.push(acc_.GetDependRoot());
    ChunkMap<GateRef, size_t> mergeVisits(GetChunk());
    std::vector<GateRef> depUses;
    while (!workList.empty()) {
        auto curDep = workList.front();
        workList.pop();
        if (acc_.IsDependSelector(curDep) && !acc_.IsLoopHead(acc_.GetState(curDep))) {
            ASSERT(acc_.GetOpCode(acc_.GetState(curDep)) == OpCode::MERGE);
            mergeVisits[curDep]++;
            if (mergeVisits.at(curDep) != acc_.GetDependCount(curDep)) {
                continue;
            }
        }
        acc_.GetDependUses(curDep, depUses);
        for (auto use : depUses) {
            workList.push(use);
        }
        TryEliminate(curDep);
    }
    for (auto gate : stateSplits_) {
        RemoveGate(gate, Circuit::NullGate());
    }
}

void EarlyElimination::RemoveTypeTrustedCheck()
{
    // eliminate type check for type trusted gate (for primitive type check)
    std::vector<GateRef> allGates;
    acc_.GetAllGates(allGates);
    ChunkQueue<GateRef> workList(GetChunk());
    ChunkVector<GateRef> checkList(GetChunk());
    for (auto gate : allGates) {
        maxId_ = std::max(maxId_, acc_.GetId(gate));
        if (IsTrustedType(gate)) {
            workList.push(gate);
        }
        if (IsPrimitiveTypeCheck(gate)) {
            checkList.emplace_back(gate);
        }
    }
    TrustedTypePropagate(workList, checkList);
}
}  // namespace panda::ecmascript::kungfu