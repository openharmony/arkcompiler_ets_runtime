/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/escape_analysis.h"

namespace panda::ecmascript::kungfu {

VirtualObject::VirtualObject(size_t numIn, Chunk* chunk)
    : fields_(chunk), users_(chunk)
{
    for (size_t i = 0; i < numIn; i++) {
        FieldLocation tmp;
        fields_.emplace_back(tmp);
    }
}

void VirtualObject::SetEscaped()
{
    escaped_ = true;
}

bool VirtualObject::IsEscaped() const
{
    return escaped_;
}

FieldLocation VirtualObject::GetField(size_t offset)
{
    constexpr size_t fieldSize = 8;
    ASSERT(offset % fieldSize == 0);
    if (offset / fieldSize >= fields_.size()) {
        return FieldLocation::Invalid();
    }
    return fields_.at(offset/fieldSize);
}

size_t FieldLocation::maxid = 0;

ChunkVector<GateRef>& VirtualObject::GetUsers()
{
    return users_;
}

void VirtualObject::ClearUsers()
{
    users_.clear();
}

void VirtualObject::AddUser(GateRef gate)
{
    users_.push_back(gate);
}

GateInfo::GateInfo(Circuit* circuit, GateRef curGate, EscapeAnalysis* escapeAnalysis, Chunk* chunk)
    : circuit_(circuit), acc_(circuit), curGate_(curGate), escapeAnalysis_(escapeAnalysis), state_(chunk)
{
    if (acc_.GetOpCode(curGate) == OpCode::DEPEND_SELECTOR) {
        state_ = MergeState(curGate);
    } else {
        ASSERT(acc_.GetDependCount(curGate) <= 1);
        if (acc_.GetDependCount(curGate) == 1) {
            state_ = escapeAnalysis_->GetOrCreateState(acc_.GetDep(curGate));
        }
    }
}

GateInfo::~GateInfo()
{
    State& preState = escapeAnalysis_->GetOrCreateState(curGate_);
    if (state_.IsMapEqual(preState) ||
        object_ != escapeAnalysis_->TryGetVirtualObject(curGate_)) {
        escapeAnalysis_->SetReplaceGate(curGate_);
    }
    escapeAnalysis_->SetState(curGate_, state_);
    
    escapeAnalysis_->SetVirtualObject(curGate_, object_);
    escapeAnalysis_->SetReplacement(curGate_, replacement_);
}

GateRef GateInfo::GetCurrentGate() const
{
    return curGate_;
}

State GateInfo::MergeState(GateRef gate)
{
    size_t numIn = acc_.GetDependCount(gate);
    State& preState = escapeAnalysis_->GetOrCreateState(acc_.GetDep(gate, 0));
    State result = preState;
    for (auto fieldValue : preState) {
        FieldLocation field = fieldValue.first;
        GateRef value = fieldValue.second;
        ASSERT(field != FieldLocation::Invalid());
        if (value != Circuit::NullGate()) {
            std::vector<GateRef> input;
            input.push_back(acc_.GetState(gate));
            input.push_back(value);
            size_t numAliveState = 1;
            bool differentFlag = false;
            for (size_t i = 1; i < numIn; i++) {
                State& inputState = escapeAnalysis_->GetOrCreateState(acc_.GetDep(gate, i));
                GateRef inputValue = inputState.GetFieldValue(field);
                if (inputValue != Circuit::NullGate()) {
                    numAliveState++;
                }
                if (inputValue != value) {
                    differentFlag = true;
                }
                input.push_back(inputValue);
            }
            
            if (numAliveState == 1 && acc_.GetOpCode(acc_.GetState(gate)) == OpCode::LOOP_BEGIN) {
                result.SetFieldValue(field, value);
            } else if (numAliveState < numIn) {
                result.SetFieldValue(field, Circuit::NullGate());
            } else {
                if (!differentFlag) {
                    result.SetFieldValue(field, value);
                } else {
                    State& oldState = escapeAnalysis_->GetOrCreateState(gate);
                    GateRef oldValue = oldState.GetFieldValue(field);
                    if (oldValue != Circuit::NullGate() &&
                        acc_.GetOpCode(oldValue) == OpCode::DEPEND_SELECTOR &&
                        acc_.GetState(oldValue) == acc_.GetState(gate)) {
                        for (size_t i = 0; i < numIn; i++) {
                            ASSERT(input[i + 1] != Circuit::NullGate());
                            if (acc_.GetValueIn(oldValue, i) != input[i + 1]) {
                                acc_.ReplaceValueIn(oldValue, input[i + 1], i);
                                escapeAnalysis_->RevisitGate(oldValue);
                            }
                        }
                        result.SetFieldValue(field, oldValue);
                    } else {
                        MachineType machineType = acc_.GetMachineType(value);
                        auto gateType = acc_.GetGateType(value);
                        const GateMetaData* meta = circuit_->ValueSelector(numIn);
                        GateRef valueSelector = circuit_->NewGate(meta, machineType, input.size(),
                                                                  input.data(), gateType);
                        result.SetFieldValue(field, valueSelector);
                        escapeAnalysis_->RevisitGate(valueSelector);
                    }
                }
            }
        }
    }
    return result;
}

GateRef GateInfo::GetFieldValue(FieldLocation field) const
{
    return state_.GetFieldValue(field);
}

void GateInfo::SetFieldValue(FieldLocation field, GateRef value)
{
    state_.SetFieldValue(field, value);
}

void GateInfo::SetEliminated()
{
    replacement_ = circuit_->DeadGate();
    object_ = nullptr;
}

void GateInfo::SetReplacement(GateRef replacement)
{
    replacement_ = replacement;
    object_ = escapeAnalysis_->TryGetVirtualObject(replacement);
}

void GateInfo::SetVirtualObject(VirtualObject* object)
{
    object_ = object;
}

void State::SetFieldValue(FieldLocation field, GateRef gate)
{
    map_[field] = gate;
}

GateRef State::GetFieldValue(FieldLocation field) const
{
    auto result = map_.find(field);
    if (result == map_.end()) {
        return Circuit::NullGate();
    }
    return result->second;
}

bool State::IsMapEqual(const State &state) const
{
    return map_.size() == state.map_.size() &&
           std::equal(map_.begin(), map_.end(), state.map_.begin());
}

void EscapeAnalysis::SetReplacement(GateRef gate, GateRef replacement)
{
    replacements_[gate] = replacement;
}

GateRef EscapeAnalysis::TryGetReplacement(GateRef gate) const
{
    if (!replacements_.count(gate)) {
        return Circuit::NullGate();
    }
    return replacements_.at(gate);
}

GateRef EscapeAnalysis::GetCurrentGate(GateRef gate) const
{
    GateRef replacement = TryGetReplacement(gate);
    if (replacement == Circuit::NullGate()) {
        return gate;
    }
    return replacement;
}

VirtualObject* EscapeAnalysis::TryGetVirtualObject(GateRef gate) const
{
    if (gateToVirtualObject_.count(gate)) {
        VirtualObject* vObj = gateToVirtualObject_.at(gate);
        return vObj;
    }
    return nullptr;
}

VirtualObject* EscapeAnalysis::TryGetVirtualObjectAndAddUser(GateRef gate, GateRef currentGate)
{
    if (gateToVirtualObject_.count(gate)) {
        VirtualObject* vObj = gateToVirtualObject_[gate];
        if (vObj != nullptr) {
            vObj->AddUser(currentGate);
        }
        return vObj;
    }
    return nullptr;
}


void EscapeAnalysis::SetVirtualObject(GateRef gate, VirtualObject* object)
{
    gateToVirtualObject_[gate] = object;
}

void EscapeAnalysis::RevisitUser(VirtualObject* vObj)
{
    auto& users = vObj->GetUsers();
    for (auto user : users) {
        if (isTraced_) {
            LOG_COMPILER(INFO) << "[escape analysis] revisit user : " <<acc_.GetId(user);
        }
        visitor_->ReVisitGate(user);
    }
    vObj->ClearUsers();
}

void EscapeAnalysis::SetEscaped(GateRef gate)
{
    if (isTraced_) {
        LOG_COMPILER(INFO) << "[escape analysis] set escaped: " << acc_.GetId(gate);
    }
    VirtualObject* vObj = TryGetVirtualObject(gate);
    if (vObj != nullptr && !vObj->IsEscaped()) {
        vObj->SetEscaped();
        RevisitUser(vObj);
    }
}

VirtualObject* EscapeAnalysis::GetOrCreateVirtualObject(size_t numIn, GateInfo* info)
{
    GateRef gate = info->GetCurrentGate();
    VirtualObject* vobj = TryGetVirtualObject(gate);
    if (vobj == nullptr) {
        vobj = chunk_->New<VirtualObject>(numIn, chunk_);
    }
    vobj->AddUser(gate);
    info->SetVirtualObject(vobj);
    return vobj;
}


GateRef EscapeAnalysis::VisitCreateObjectWithBuffer(GateRef gate, GateInfo* info)
{
    constexpr size_t startIn = 4; // 4 : start of props
    constexpr size_t fieldSize = 8; // 8 : bytes per field
    constexpr size_t stride = 2; // 2: offset and value
    auto numIn = acc_.GetNumValueIn(gate);
    size_t size = acc_.GetConstantValue(acc_.GetValueIn(gate, 0)) / fieldSize;
    VirtualObject* vObj = GetOrCreateVirtualObject(size, info);
    
    for (size_t i = startIn; i < numIn; i += stride) {
        GateRef value = acc_.GetValueIn(gate, i);
        GateRef offset = acc_.GetValueIn(gate, i + 1);
        if (vObj != nullptr && !vObj->IsEscaped() &&
            vObj->GetField(acc_.GetConstantValue(offset)) != FieldLocation::Invalid()) {
            info->SetFieldValue(vObj->GetField(acc_.GetConstantValue(offset)), value);
        } else {
            SetEscaped(value);
            SetEscaped(gate);
        }
    }
    info->SetVirtualObject(vObj);
    return replaceGate_;
}

GateRef EscapeAnalysis::VisitLoadProperty(GateRef gate, GateInfo* info)
{
    GateRef object = acc_.GetValueIn(gate, 0);
    GateRef offset = acc_.GetValueIn(gate, 1);
    VirtualObject* vObj = TryGetVirtualObjectAndAddUser(object, gate);
    
    PropertyLookupResult plr(acc_.GetConstantValue(offset));

    if (vObj != nullptr && !vObj->IsEscaped() && vObj->GetField(plr.GetOffset()) != FieldLocation::Invalid()) {
        GateRef value = info->GetFieldValue(vObj->GetField(plr.GetOffset()));
        if (value != Circuit::NullGate()) {
            if (isTraced_) {
                LOG_COMPILER(INFO) << "[escape analysis] replace" << acc_.GetId(gate) << " with " << acc_.GetId(value);
            }
            info->SetReplacement(value);
        } else {
            SetEscaped(object);
        }
    } else {
        SetEscaped(object);
    }
    return replaceGate_;
}

GateRef EscapeAnalysis::VisitLoadConstOffset(GateRef gate, GateInfo* info)
{
    GateRef object = acc_.GetValueIn(gate, 0);
    size_t offset = acc_.GetOffset(gate);
    VirtualObject* vObj = TryGetVirtualObjectAndAddUser(object, gate);

    if (vObj != nullptr && !vObj->IsEscaped() && vObj->GetField(offset) != FieldLocation::Invalid()) {
        GateRef value = info->GetFieldValue(vObj->GetField(offset));
        if (value != Circuit::NullGate()) {
            if (isTraced_) {
                LOG_COMPILER(INFO) << "[escape analysis] replace " <<
                                      acc_.GetId(gate) << " with " << acc_.GetId(value);
            }
            info->SetReplacement(value);
        } else {
            SetEscaped(object);
        }
    } else {
        SetEscaped(object);
    }
    return replaceGate_;
}


GateRef EscapeAnalysis::VisitStoreProperty(GateRef gate, GateInfo* info)
{
    GateRef object = acc_.GetValueIn(gate, 0);
    GateRef offset = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    VirtualObject* vObj = TryGetVirtualObjectAndAddUser(object, gate);
    PropertyLookupResult plr(acc_.GetConstantValue(offset));
    
    if (vObj != nullptr && !vObj->IsEscaped() && vObj->GetField(plr.GetOffset()) != FieldLocation::Invalid()) {
        info->SetFieldValue(vObj->GetField(plr.GetOffset()), value);
        info->SetEliminated();
    } else {
        SetEscaped(value);
        SetEscaped(object);
    }
    return replaceGate_;
}

GateRef EscapeAnalysis::VisitObjectTypeCheck(GateRef gate, GateInfo* info)
{
    info->SetVirtualObject(TryGetVirtualObject(acc_.GetValueIn(gate)));
    return Circuit::NullGate();
}

State& EscapeAnalysis::GetOrCreateState(GateRef gate)
{
    auto it = gateToState_.find(gate);
    if (it == gateToState_.end()) {
        State tmp(chunk_);
        auto result = gateToState_.insert(std::make_pair(gate, std::move(tmp)));
        return result.first->second;
    }
    return it->second;
}

void EscapeAnalysis::SetState(GateRef gate, State state)
{
    auto it = gateToState_.find(gate);
    if (it == gateToState_.end()) {
        gateToState_.insert(std::make_pair(gate, std::move(state)));
    } else {
        it->second = state;
    }
}

void EscapeAnalysis::RevisitGate(GateRef gate)
{
    visitor_->ReVisitGate(gate);
}

void EscapeAnalysis::SetReplaceGate(GateRef gate)
{
    replaceGate_ = gate;
}

GateRef EscapeAnalysis::VisitGate(GateRef gate)
{
    GateInfo info(circuit_, gate, this, chunk_);
    auto opcode = acc_.GetOpCode(gate);
    replaceGate_ = Circuit::NullGate();
    switch (opcode) {
        case OpCode::STORE_PROPERTY:
        case OpCode::STORE_PROPERTY_NO_BARRIER:
            return VisitStoreProperty(gate, &info);
        case OpCode::LOAD_PROPERTY:
            return VisitLoadProperty(gate, &info);
        case OpCode::LOAD_CONST_OFFSET:
        case OpCode::LOAD_HCLASS_CONST_OFFSET:
            return VisitLoadConstOffset(gate, &info);
        case OpCode::TYPED_CREATE_OBJ_WITH_BUFFER:
            return VisitCreateObjectWithBuffer(gate, &info);
        case OpCode::OBJECT_TYPE_CHECK:
        case OpCode::CHECK_AND_CONVERT:
            return VisitObjectTypeCheck(gate, &info);
        case OpCode::FRAME_VALUES:
        case OpCode::STATE_SPLIT:
        case OpCode::FRAME_STATE:
            break;
        default : {
            size_t numIns = acc_.GetNumValueIn(gate);
            for (size_t i = 0; i < numIns; i++) {
                GateRef in = GetCurrentGate(acc_.GetValueIn(gate, i));
                SetEscaped(in);
            }
        }
    }
    return Circuit::NullGate();
}

}