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

GateRef DependChainInfo::LookUpElement(ElementInfo info) const
{
    if ((elementMap_ != nullptr) && (elementMap_->count(info) > 0)) {
        return elementMap_->at(info);
    } else {
        return Circuit::NullGate();
    }
}

GateRef DependChainInfo::LookUpProperty(PropertyInfo info) const
{
    if ((propertyMap_ != nullptr) && (propertyMap_->count(info) > 0)) {
        return propertyMap_->at(info);
    } else {
        return Circuit::NullGate();
    }
}

GateRef DependChainInfo::LookUpArrayLength(ArrayLengthInfo info) const
{
    if ((arrayLengthMap_ != nullptr) && (arrayLengthMap_->count(info) > 0)) {
        return arrayLengthMap_->at(info);
    } else {
        return Circuit::NullGate();
    }
}

bool DependChainInfo::LookUpGateTypeCheck(GateTypeCheckInfo info) const
{
    return (gateTypeCheckSet_ != nullptr) && (gateTypeCheckSet_->count(info) > 0);
}

GateTypeCheckInfo DependChainInfo::LookUpGateTypeCheck(GateRef gate) const
{
    if (gateTypeCheckSet_ == nullptr) {
        return GateTypeCheckInfo();
    }
    auto findFunc = [] (const GateTypeCheckInfo& info, const GateRef &value) {
        return info.GetValue() < value;
    };
    const auto &it = std::lower_bound(gateTypeCheckSet_->begin(),
        gateTypeCheckSet_->end(), gate, findFunc);
    if (it != gateTypeCheckSet_->end() && it->GetValue() == gate) {
        return *it;
    }
    return GateTypeCheckInfo();
}

bool DependChainInfo::LookUpInt32OverflowCheck(Int32OverflowCheckInfo info) const
{
    return (int32OverflowCheckSet_ != nullptr) && (int32OverflowCheckSet_->count(info) > 0);
}

bool DependChainInfo::LookUpStableArrayCheck(StableArrayCheckInfo info) const
{
    return (stableArrayCheckSet_ != nullptr) && (stableArrayCheckSet_->count(info) > 0);
}

bool DependChainInfo::LookUpObjectTypeCheck(ObjectTypeCheckInfo info) const
{
    return (objectTypeCheckSet_ != nullptr) && (objectTypeCheckSet_->count(info) > 0);
}

bool DependChainInfo::LookUpIndexCheck(IndexCheckInfo info) const
{
    return (indexCheckSet_ != nullptr) && (indexCheckSet_->count(info) > 0);
}

bool DependChainInfo::LookUpTypedCallCheck(TypedCallCheckInfo info) const
{
    return (typedCallCheckSet_ != nullptr) && (typedCallCheckSet_->count(info) > 0);
}

GateRef DependChainInfo::LookUpFrameState() const
{
    return frameState_;
}

DependChainInfo* DependChainInfo::UpdateProperty(PropertyInfo info, GateRef gate)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (propertyMap_ != nullptr) {
        that->propertyMap_ = new ChunkMap<PropertyInfo, GateRef>(*propertyMap_);
    } else {
        that->propertyMap_ = new ChunkMap<PropertyInfo, GateRef>(chunk_);
    }
    that->propertyMap_->insert(std::make_pair(info, gate));
    return that;
}

DependChainInfo* DependChainInfo::UpdateElement(ElementInfo info, GateRef gate)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (elementMap_ != nullptr) {
        that->elementMap_ = new ChunkMap<ElementInfo, GateRef>(*elementMap_);
    } else {
        that->elementMap_ = new ChunkMap<ElementInfo, GateRef>(chunk_);
    }
    that->elementMap_->insert(std::make_pair(info, gate));
    return that;
}

DependChainInfo* DependChainInfo::UpdateArrayLength(ArrayLengthInfo info, GateRef gate)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (arrayLengthMap_ != nullptr) {
        that->arrayLengthMap_ = new ChunkMap<ArrayLengthInfo, GateRef>(*arrayLengthMap_);
    } else {
        that->arrayLengthMap_ = new ChunkMap<ArrayLengthInfo, GateRef>(chunk_);
    }
    that->arrayLengthMap_->insert(std::make_pair(info, gate));
    return that;
}

DependChainInfo* DependChainInfo::UpdateGateTypeCheck(GateTypeCheckInfo info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (gateTypeCheckSet_ != nullptr) {
        that->gateTypeCheckSet_ = new ChunkSet<GateTypeCheckInfo>(*gateTypeCheckSet_);
    } else {
        that->gateTypeCheckSet_ = new ChunkSet<GateTypeCheckInfo>(chunk_);
    }
    that->gateTypeCheckSet_->insert(info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateInt32OverflowCheck(Int32OverflowCheckInfo info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (int32OverflowCheckSet_ != nullptr) {
        that->int32OverflowCheckSet_ = new ChunkSet<Int32OverflowCheckInfo>(*int32OverflowCheckSet_);
    } else {
        that->int32OverflowCheckSet_ = new ChunkSet<Int32OverflowCheckInfo>(chunk_);
    }
    that->int32OverflowCheckSet_->insert(info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateStableArrayCheck(StableArrayCheckInfo info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (stableArrayCheckSet_ != nullptr) {
        that->stableArrayCheckSet_ = new ChunkSet<StableArrayCheckInfo>(*stableArrayCheckSet_);
    } else {
        that->stableArrayCheckSet_ = new ChunkSet<StableArrayCheckInfo>(chunk_);
    }
    that->stableArrayCheckSet_->insert(info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateObjectTypeCheck(ObjectTypeCheckInfo info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (objectTypeCheckSet_ != nullptr) {
        that->objectTypeCheckSet_ = new ChunkSet<ObjectTypeCheckInfo>(*objectTypeCheckSet_);
    } else {
        that->objectTypeCheckSet_ = new ChunkSet<ObjectTypeCheckInfo>(chunk_);
    }
    that->objectTypeCheckSet_->insert(info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateIndexCheck(IndexCheckInfo info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (indexCheckSet_ != nullptr) {
        that->indexCheckSet_ = new ChunkSet<IndexCheckInfo>(*indexCheckSet_);
    } else {
        that->indexCheckSet_ = new ChunkSet<IndexCheckInfo>(chunk_);
    }
    that->indexCheckSet_->insert(info);
    return that;
}

DependChainInfo* DependChainInfo::UpdateTypedCallCheck(TypedCallCheckInfo info)
{
    DependChainInfo* that = new (chunk_) DependChainInfo(*this);
    if (typedCallCheckSet_ != nullptr) {
        that->typedCallCheckSet_ = new ChunkSet<TypedCallCheckInfo>(*typedCallCheckSet_);
    } else {
        that->typedCallCheckSet_ = new ChunkSet<TypedCallCheckInfo>(chunk_);
    }
    that->typedCallCheckSet_->insert(info);
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
    // save gateTypeCheckSet_ and int32OverflowCheckSet_ since these checks have no side effect
    DependChainInfo* that = new (chunk_) DependChainInfo(chunk_);
    that->gateTypeCheckSet_ = gateTypeCheckSet_;
    that->int32OverflowCheckSet_ = int32OverflowCheckSet_;
    return that;
}

bool DependChainInfo::Empty() const {
    return (elementMap_ == nullptr) &&
           (propertyMap_ == nullptr) &&
           (arrayLengthMap_ == nullptr) &&
           (gateTypeCheckSet_ == nullptr) &&
           (int32OverflowCheckSet_ == nullptr) &&
           (stableArrayCheckSet_ == nullptr) &&
           (objectTypeCheckSet_ == nullptr) &&
           (indexCheckSet_ == nullptr) &&
           (typedCallCheckSet_ == nullptr) &&
           (frameState_ == Circuit::NullGate());
}

template<typename K, typename V>
bool DependChainInfo::EqualsMap(ChunkMap<K, V>* thisMap, ChunkMap<K, V>* thatMap)
{
    if (thisMap == thatMap) {
        return true;
    } else if (thisMap == nullptr || thatMap == nullptr) {
        return false;
    } else {
        return *thisMap == *thatMap;
    }
}

template<typename K>
bool DependChainInfo::EqualsSet(ChunkSet<K>* thisSet, ChunkSet<K>* thatSet)
{
    if (thisSet == thatSet) {
        return true;
    } else if (thisSet == nullptr || thatSet == nullptr) {
        return false;
    } else {
        return *thisSet == *thatSet;
    }
}

bool DependChainInfo::Equals(DependChainInfo* that)
{
    if (this == that) return true;
    if (that == nullptr) return false;
    if (frameState_ != that->frameState_) {
        return false;
    }
    if (!EqualsMap<ElementInfo, GateRef>(elementMap_, that->elementMap_)) {
        return false;
    }
    if (!EqualsMap<PropertyInfo, GateRef>(propertyMap_, that->propertyMap_)) {
        return false;
    }
    if (!EqualsMap<ArrayLengthInfo, GateRef>(arrayLengthMap_, that->arrayLengthMap_)) {
        return false;
    }
    if (!EqualsSet<GateTypeCheckInfo>(gateTypeCheckSet_, that->gateTypeCheckSet_)) {
        return false;
    }
    if (!EqualsSet<Int32OverflowCheckInfo>(int32OverflowCheckSet_, that->int32OverflowCheckSet_)) {
        return false;
    }
    if (!EqualsSet<StableArrayCheckInfo>(stableArrayCheckSet_, that->stableArrayCheckSet_)) {
        return false;
    }
    if (!EqualsSet<ObjectTypeCheckInfo>(objectTypeCheckSet_, that->objectTypeCheckSet_)) {
        return false;
    }
    if (!EqualsSet<IndexCheckInfo>(indexCheckSet_, that->indexCheckSet_)) {
        return false;
    }
    if (!EqualsSet<TypedCallCheckInfo>(typedCallCheckSet_, that->typedCallCheckSet_)) {
        return false;
    }
    return true;
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
    if (Equals(that)) {
        return that;
    }
    DependChainInfo* newInfo = new (chunk_) DependChainInfo(*this);
    newInfo->elementMap_ = MergeMap<ElementInfo, GateRef>(elementMap_, that->elementMap_);
    newInfo->propertyMap_ = MergeMap<PropertyInfo, GateRef>(propertyMap_, that->propertyMap_);
    newInfo->arrayLengthMap_ = MergeMap<ArrayLengthInfo, GateRef>(arrayLengthMap_, that->arrayLengthMap_);
    newInfo->gateTypeCheckSet_ =
        MergeSet<GateTypeCheckInfo>(gateTypeCheckSet_, that->gateTypeCheckSet_);
    newInfo->int32OverflowCheckSet_ =
        MergeSet<Int32OverflowCheckInfo>(int32OverflowCheckSet_, that->int32OverflowCheckSet_);
    newInfo->stableArrayCheckSet_ = MergeSet<StableArrayCheckInfo>(stableArrayCheckSet_, that->stableArrayCheckSet_);
    newInfo->objectTypeCheckSet_ = MergeSet<ObjectTypeCheckInfo>(objectTypeCheckSet_, that->objectTypeCheckSet_);
    newInfo->indexCheckSet_ = MergeSet<IndexCheckInfo>(indexCheckSet_, that->indexCheckSet_);
    newInfo->typedCallCheckSet_ = MergeSet<TypedCallCheckInfo>(typedCallCheckSet_, that->typedCallCheckSet_);
    newInfo->frameState_ = frameState_ == that->frameState_ ? frameState_ : Circuit::NullGate();
    return newInfo;
}

void EarlyElimination::Run()
{
    dependInfos_.resize(circuit_->GetMaxGateId() + 1, nullptr); // 1: +1 for size
    GateRef entry = acc_.GetDependRoot();
    VisitDependEntry(entry);
    VisitGraph();

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After early eliminating "
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
        if (!visited.empty()) return false;
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

ElementInfo EarlyElimination::GetElementInfo(GateRef gate) const
{
    auto op = acc_.GetTypedLoadOp(gate);
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    return ElementInfo(op, v0, v1);
}

PropertyInfo EarlyElimination::GetPropertyInfo(GateRef gate) const
{
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    return PropertyInfo(v0, v1);
}

ArrayLengthInfo EarlyElimination::GetArrayLengthInfo(GateRef gate) const
{
    auto v0 = acc_.GetValueIn(gate, 0);
    return ArrayLengthInfo(v0);
}

Int32OverflowCheckInfo EarlyElimination::GetInt32OverflowCheckInfo(GateRef gate) const
{
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    auto op = accessor.GetTypedUnOp();
    auto v0 = acc_.GetValueIn(gate, 0);
    return Int32OverflowCheckInfo(op, v0);
}

StableArrayCheckInfo EarlyElimination::GetStableArrayCheckInfo(GateRef gate) const
{
    auto v0 =  acc_.GetValueIn(gate, 0);
    return StableArrayCheckInfo(v0);
}

IndexCheckInfo EarlyElimination::GetIndexCheckInfo(GateRef gate) const
{
    auto type = acc_.GetParamGateType(gate);
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    return IndexCheckInfo(type, v0, v1);
}

TypedCallCheckInfo EarlyElimination::GetTypedCallCheckInfo(GateRef gate) const
{
    auto v0 = acc_.GetValueIn(gate, 0);
    auto v1 = acc_.GetValueIn(gate, 1);
    auto v2 = acc_.GetValueIn(gate, 2);
    return TypedCallCheckInfo(v0, v1, v2);
}

GateRef EarlyElimination::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::LOAD_PROPERTY:
            return TryEliminateProperty(gate);
        case OpCode::LOAD_ELEMENT:
            return TryEliminateElement(gate);
        case OpCode::LOAD_ARRAY_LENGTH:
            return TryEliminateArrayLength(gate);
        case OpCode::TYPED_ARRAY_CHECK:
        case OpCode::OBJECT_TYPE_CHECK:
            return TryEliminateObjectTypeCheck(gate);
        case OpCode::PRIMITIVE_TYPE_CHECK:
            return TryEliminateGateTypeCheck(gate);
        case OpCode::INT32_OVERFLOW_CHECK:
            return TryEliminateInt32OverflowCheck(gate);
        case OpCode::STABLE_ARRAY_CHECK:
            return TryEliminateStableArrayCheck(gate);
        case OpCode::INDEX_CHECK:
            return TryEliminateIndexCheck(gate);
        case OpCode::TYPED_CALL_CHECK:
            return TryEliminateTypedCallCheck(gate);
        case OpCode::STATE_SPLIT:
            return TryEliminateStateSplitAndFrameState(gate);
        case OpCode::DEPEND_SELECTOR:
            return TryEliminateDependSelector(gate);
        case OpCode::TYPED_BINARY_OP:
            return VisitTypedBinaryOp(gate);
        case OpCode::TYPED_UNARY_OP:
            return VisitTypedUnaryOp(gate);
        default:
            if (acc_.GetDependCount(gate) == 1) { // 1: depend in is 1
                return TryEliminateOther(gate);
            }
    }
    return Circuit::NullGate();
}

GateRef EarlyElimination::VisitTypedBinaryOp(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }

    GateType valueType = acc_.GetGateType(gate);
    if (!valueType.IsNumberType() || valueType.IsIntType()) {
        return UpdateDependInfo(gate, dependInfo);
    }
    auto info = GateTypeCheckInfo(gate, valueType);
    if (dependInfo->LookUpGateTypeCheck(info)) {
        return UpdateDependInfo(gate, dependInfo);
    }
    dependInfo = dependInfo->UpdateGateTypeCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::VisitTypedUnaryOp(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    TypedUnaryAccessor accessor(acc_.TryGetValue(gate));
    GateType valueType = accessor.GetTypeValue();
    if (!valueType.IsNumberType() || valueType.IsIntType()) {
        return UpdateDependInfo(gate, dependInfo);
    }
    auto info = GateTypeCheckInfo(gate, valueType);
    if (dependInfo->LookUpGateTypeCheck(info)) {
        return UpdateDependInfo(gate, dependInfo);
    }
    dependInfo = dependInfo->UpdateGateTypeCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::VisitDependEntry(GateRef gate)
{
    auto emptyInfo = new (GetChunk()) DependChainInfo(GetChunk());
    return UpdateDependInfo(gate, emptyInfo);
}

GateRef EarlyElimination::TryEliminateElement(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto info = GetElementInfo(gate);
    auto preGate = dependInfo->LookUpElement(info);
    if (preGate != Circuit::NullGate()) {
        return preGate;
    }
    dependInfo = dependInfo->UpdateElement(info, gate);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateProperty(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto info = GetPropertyInfo(gate);
    auto preGate = dependInfo->LookUpProperty(info);
    if (preGate != Circuit::NullGate()) {
        return preGate;
    }
    dependInfo = dependInfo->UpdateProperty(info, gate);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateArrayLength(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto info = GetArrayLengthInfo(gate);
    auto preGate = dependInfo->LookUpArrayLength(info);
    if (preGate != Circuit::NullGate()) {
        return preGate;
    }

    // update arrayLength as number Type
    auto typeInfo = GateTypeCheckInfo(gate, GateType::NumberType());
    if (!dependInfo->LookUpGateTypeCheck(typeInfo)) {
        dependInfo = dependInfo->UpdateGateTypeCheck(typeInfo);
    }

    dependInfo = dependInfo->UpdateArrayLength(info, gate);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateObjectTypeCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }

    auto type = acc_.GetParamGateType(gate);
    auto value = acc_.GetValueIn(gate, 0);
    auto info = ObjectTypeCheckInfo(type, value);
    if (dependInfo->LookUpObjectTypeCheck(info)) {
        return circuit_->DeadGate();
    }

    dependInfo = dependInfo->UpdateObjectTypeCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateGateTypeCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }

    auto type = acc_.GetParamGateType(gate);
    auto value = acc_.GetValueIn(gate, 0);
    auto info = GateTypeCheckInfo(value, type);
    if (dependInfo->LookUpGateTypeCheck(info)) {
        return circuit_->DeadGate();
    }

    dependInfo = dependInfo->UpdateGateTypeCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateInt32OverflowCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto info = GetInt32OverflowCheckInfo(gate);
    if (dependInfo->LookUpInt32OverflowCheck(info)) {
        return circuit_->DeadGate();
    }
    dependInfo = dependInfo->UpdateInt32OverflowCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateStableArrayCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto info = GetStableArrayCheckInfo(gate);
    if (dependInfo->LookUpStableArrayCheck(info)) {
        return circuit_->DeadGate();
    }
    dependInfo = dependInfo->UpdateStableArrayCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateIndexCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto info = GetIndexCheckInfo(gate);
    if (dependInfo->LookUpIndexCheck(info)) {
        return circuit_->DeadGate();
    }
    dependInfo = dependInfo->UpdateIndexCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateTypedCallCheck(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto info = GetTypedCallCheckInfo(gate);
    if (dependInfo->LookUpTypedCallCheck(info)) {
        return circuit_->DeadGate();
    }
    dependInfo = dependInfo->UpdateTypedCallCheck(info);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateStateSplitAndFrameState(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto frameState = dependInfo->LookUpFrameState();
    auto curFrameState = acc_.GetFrameState(gate);
    if (frameState != Circuit::NullGate()) {
        acc_.UpdateAllUses(curFrameState, frameState);
        return Circuit::NullGate();
    }
    dependInfo = dependInfo->UpdateFrameState(curFrameState);
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateOther(GateRef gate)
{
    ASSERT(acc_.GetDependCount(gate) == 1);
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    if (!acc_.IsNotWrite(gate)) {
        dependInfo = dependInfo->UpdateWrite();
    }
    return UpdateDependInfo(gate, dependInfo);
}

GateRef EarlyElimination::TryEliminateDependSelector(GateRef gate)
{
    auto depIn = acc_.GetDep(gate);
    auto dependInfo = dependInfos_[acc_.GetId(depIn)];
    if (dependInfo == nullptr) {
        return Circuit::NullGate();
    }
    auto state = acc_.GetState(gate);
    if (acc_.IsLoopHead(state)) {
        if (IsSideEffectLoop(gate)) {
            dependInfo = dependInfo->UpdateWrite();
        }
        auto loopBackDepend = acc_.GetDep(gate, 1); // 1: loop back depend
        auto tempInfo = dependInfos_[acc_.GetId(loopBackDepend)];
        if (tempInfo == nullptr) {
            return UpdateDependInfo(gate, dependInfo);
        }
    } else {
        auto dependCount = acc_.GetDependCount(gate);
        for (size_t i = 1; i < dependCount; ++i) {
            auto depend = acc_.GetDep(gate, i);
            auto tempInfo = dependInfos_[acc_.GetId(depend)];
            if (tempInfo == nullptr) {
                return Circuit::NullGate();
            }
        }
        for (size_t i = 1; i < dependCount; ++i) {
            auto depend = acc_.GetDep(gate, i);
            auto tempInfo = dependInfos_[acc_.GetId(depend)];
            dependInfo = dependInfo->Merge(tempInfo);
        }
    }

    auto uses = acc_.Uses(state);
    for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
        auto opcode = acc_.GetOpCode(*useIt);
        if (opcode == OpCode::VALUE_SELECTOR) {
            dependInfo = UpadateDependInfoForPhi(dependInfo, gate, *useIt);
        }
    }

    return UpdateDependInfo(gate, dependInfo);
}

DependChainInfo* EarlyElimination::UpadateDependInfoForPhi(
    DependChainInfo* outDependInfo, GateRef dependPhi, GateRef phi)
{
    GateRef value = acc_.GetValueIn(phi, 0); // 0: value 0
    GateRef depend = acc_.GetDep(dependPhi, 0); // 0: depend 0
    auto dependInfo = dependInfos_[acc_.GetId(depend)];
    auto checkInfo = dependInfo->LookUpGateTypeCheck(value);
    if (checkInfo.IsEmpty()) {
        return outDependInfo;
    }

    auto numValueIns = acc_.GetNumValueIn(phi);
    for (size_t i = 1; i < numValueIns; i++) {
        GateRef input = acc_.GetValueIn(phi, i);
        GateRef dependInput = acc_.GetDep(dependPhi, i); // 0: depend 0
        dependInfo = dependInfos_[acc_.GetId(dependInput)];
        auto otherInfo = dependInfo->LookUpGateTypeCheck(input);
        if (otherInfo.IsEmpty()) {
            return outDependInfo;
        }
        if (checkInfo.GetType() != otherInfo.GetType()) {
            return outDependInfo;
        }
    }

    checkInfo = GateTypeCheckInfo(phi, checkInfo.GetType());
    return outDependInfo->UpdateGateTypeCheck(checkInfo);
}

GateRef EarlyElimination::UpdateDependInfo(GateRef gate, DependChainInfo* dependInfo)
{
    ASSERT(dependInfo != nullptr);
    auto oldDependInfo = dependInfos_[acc_.GetId(gate)];
    if (dependInfo->Equals(oldDependInfo)) {
        return Circuit::NullGate();
    }
    dependInfos_[acc_.GetId(gate)] = dependInfo;
    return gate;
}

}  // namespace panda::ecmascript::kungfu