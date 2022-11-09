/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/circuit_builder.h"

namespace panda::ecmascript::kungfu {
using UseIterator = GateAccessor::UseIterator;

size_t GateAccessor::GetNumIns(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetNumIns();
}

MarkCode GateAccessor::GetMark(GateRef gate) const
{
    return circuit_->GetMark(gate);
}

void GateAccessor::SetMark(GateRef gate, MarkCode mark)
{
    circuit_->SetMark(gate, mark);
}

bool GateAccessor::IsFinished(GateRef gate) const
{
    return GetMark(gate) == MarkCode::FINISHED;
}

bool GateAccessor::IsVisited(GateRef gate) const
{
    return GetMark(gate) == MarkCode::VISITED;
}

bool GateAccessor::IsNotMarked(GateRef gate) const
{
    return GetMark(gate) == MarkCode::NO_MARK;
}

void GateAccessor::SetFinished(GateRef gate)
{
    SetMark(gate, MarkCode::FINISHED);
}

void GateAccessor::SetVisited(GateRef gate)
{
    SetMark(gate, MarkCode::VISITED);
}

OpCode GateAccessor::GetOpCode(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetOpCode();
}

BitField GateAccessor::GetBitField(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetBitField();
}

uint32_t GateAccessor::GetBytecodeIndex(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::JS_BYTECODE);
    BitField bitField = GetBitField(gate);
    return GateBitFieldAccessor::GetBytecodeIndex(bitField);
}

EcmaOpcode GateAccessor::GetByteCodeOpcode(GateRef gate) const
{
    ASSERT(GetOpCode(gate) == OpCode::JS_BYTECODE);
    BitField bitField = GetBitField(gate);
    return GateBitFieldAccessor::GetByteCodeOpcode(bitField);
}

void GateAccessor::Print(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->Print();
}

void GateAccessor::ShortPrint(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->ShortPrint();
}

GateId GateAccessor::GetId(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetId();
}

void GateAccessor::SetOpCode(GateRef gate, OpCode::Op opcode)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->SetOpCode(OpCode(opcode));
}

void GateAccessor::SetBitField(GateRef gate, BitField bitField)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    gatePtr->SetBitField(bitField);
}

GateRef GateAccessor::GetValueIn(GateRef gate, size_t idx) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetInValueCount());
    size_t valueIndex = gatePtr->GetStateCount() + gatePtr->GetDependCount();
    return circuit_->GetIn(gate, valueIndex + idx);
}

size_t GateAccessor::GetNumValueIn(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    return gatePtr->GetInValueCount();
}

bool GateAccessor::IsGCRelated(GateRef gate) const
{
    return GetGateType(gate).IsGCRelated();
}

GateRef GateAccessor::GetIn(GateRef gate, size_t idx) const
{
    return circuit_->GetIn(gate, idx);
}

GateRef GateAccessor::GetState(GateRef gate, size_t idx) const
{
    ASSERT(idx < circuit_->LoadGatePtr(gate)->GetStateCount());
    return circuit_->GetIn(gate, idx);
}

void GateAccessor::GetInVector(GateRef gate, std::vector<GateRef>& ins) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    for (size_t idx = 0; idx < curGate->GetNumIns(); idx++) {
        ins.push_back(circuit_->GetGateRef(curGate->GetInGateConst(idx)));
    }
}

void GateAccessor::GetInStateVector(GateRef gate, std::vector<GateRef>& ins) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    for (size_t idx = 0; idx < curGate->GetStateCount(); idx++) {
        ins.push_back(circuit_->GetGateRef(curGate->GetInGateConst(idx)));
    }
}

void GateAccessor::GetOutVector(GateRef gate, std::vector<GateRef>& outs) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    if (!curGate->IsFirstOutNull()) {
        const Out *curOut = curGate->GetFirstOutConst();
        GateRef ref = circuit_->GetGateRef(curOut->GetGateConst());
        outs.push_back(ref);
        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOutConst();
            ref = circuit_->GetGateRef(curOut->GetGateConst());
            outs.push_back(ref);
        }
    }
}

void GateAccessor::GetOutStateVector(GateRef gate, std::vector<GateRef>& outStates) const
{
    const Gate *curGate = circuit_->LoadGatePtrConst(gate);
    if (!curGate->IsFirstOutNull()) {
        const Out *curOut = curGate->GetFirstOutConst();
        GateRef ref = circuit_->GetGateRef(curOut->GetGateConst());
        if (circuit_->GetOpCode(ref).IsState()) {
            outStates.push_back(ref);
        }
        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOutConst();
            ref = circuit_->GetGateRef(curOut->GetGateConst());
            if (circuit_->GetOpCode(ref).IsState()) {
                outStates.push_back(ref);
            }
        }
    }
}

void GateAccessor::GetAllGates(std::vector<GateRef>& gates) const
{
    circuit_->GetAllGates(gates);
}

bool GateAccessor::IsInGateNull(GateRef gate, size_t idx) const
{
    return circuit_->IsInGateNull(gate, idx);
}

bool GateAccessor::IsSelector(GateRef g) const
{
    return GetOpCode(g) == OpCode::VALUE_SELECTOR;
}

bool GateAccessor::IsControlCase(GateRef gate) const
{
    return circuit_->IsControlCase(gate);
}

bool GateAccessor::IsLoopHead(GateRef gate) const
{
    return circuit_->IsLoopHead(gate);
}

bool GateAccessor::IsLoopBack(GateRef gate) const
{
    return GetOpCode(gate) == OpCode::LOOP_BACK;
}

bool GateAccessor::IsState(GateRef gate) const
{
    return GetOpCode(gate).IsState();
}

bool GateAccessor::IsConstant(GateRef gate) const
{
    return GetOpCode(gate).IsConstant();
}

bool GateAccessor::IsConstantValue(GateRef gate, uint64_t value) const
{
    auto isConstant = IsConstant(gate);
    if (isConstant) {
        BitField bitField = GetBitField(gate);
        return GateBitFieldAccessor::GetConstantValue(bitField) == value;
    }
    return false;
}

bool GateAccessor::IsTypedOperator(GateRef gate) const
{
    return GetOpCode(gate).IsTypedOperator();
}

bool GateAccessor::IsSchedulable(GateRef gate) const
{
    return GetOpCode(gate).IsSchedulable();
}

GateRef GateAccessor::GetDep(GateRef gate, size_t idx) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetDependCount());
    size_t dependIndex = gatePtr->GetStateCount();
    return circuit_->GetIn(gate, dependIndex + idx);
}

size_t GateAccessor::GetImmediateId(GateRef gate) const
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(gatePtr->GetGateType() == GateType::NJSValue());
    ASSERT(gatePtr->GetOpCode() == OpCode::CONSTANT);
    ASSERT(gatePtr->GetMachineType() == MachineType::I64);
    size_t imm = gatePtr->GetBitField();
    return imm;
}

void GateAccessor::SetDep(GateRef gate, GateRef depGate, size_t idx)
{
    Gate *gatePtr = circuit_->LoadGatePtr(gate);
    ASSERT(idx < gatePtr->GetDependCount());
    size_t dependIndex = gatePtr->GetStateCount();
    gatePtr->ModifyIn(dependIndex + idx, circuit_->LoadGatePtr(depGate));
}

UseIterator GateAccessor::ReplaceIn(const UseIterator &useIt, GateRef replaceGate)
{
    UseIterator next = useIt;
    next++;
    Gate *curGatePtr = circuit_->LoadGatePtr(*useIt);
    Gate *replaceGatePtr = circuit_->LoadGatePtr(replaceGate);
    curGatePtr->ModifyIn(useIt.GetIndex(), replaceGatePtr);
    return next;
}

GateType GateAccessor::GetGateType(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetGateType();
}

void GateAccessor::SetGateType(GateRef gate, GateType gt)
{
    circuit_->LoadGatePtr(gate)->SetGateType(gt);
}

UseIterator GateAccessor::DeleteExceptionDep(const UseIterator &useIt)
{
    auto next = useIt;
    next++;
    ASSERT(GetOpCode(*useIt) == OpCode::RETURN || GetOpCode(*useIt) == OpCode::DEPEND_SELECTOR);
    if (GetOpCode(*useIt) == OpCode::RETURN) {
        DeleteGate(useIt);
    } else {
        size_t idx = useIt.GetIndex();
        auto merge = GetState(*useIt, 0);
        circuit_->DecreaseIn(merge, idx - 1);
        auto mergeUses = Uses(merge);
        for (auto useGate : mergeUses) {
            if (circuit_->GetOpCode(useGate) == OpCode::VALUE_SELECTOR) {
                circuit_->DecreaseIn(useGate, idx);
            }
        }
        DecreaseIn(useIt);
    }
    return next;
}

UseIterator GateAccessor::DeleteGate(const UseIterator &useIt)
{
    auto next = useIt;
    next++;
    circuit_->DeleteGate(*useIt);
    return next;
}

void GateAccessor::DecreaseIn(const UseIterator &useIt)
{
    size_t idx = useIt.GetIndex();
    circuit_->DecreaseIn(*useIt, idx);
}


void GateAccessor::DecreaseIn(GateRef gate, size_t index)
{
    circuit_->DecreaseIn(gate, index);
}

void GateAccessor::NewIn(GateRef gate, size_t idx, GateRef in)
{
    circuit_->NewIn(gate, idx, in);
}

size_t GateAccessor::GetStateCount(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetStateCount();
}

size_t GateAccessor::GetDependCount(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetDependCount();
}

size_t GateAccessor::GetInValueCount(GateRef gate) const
{
    return circuit_->LoadGatePtr(gate)->GetInValueCount();
}

void GateAccessor::UpdateAllUses(GateRef oldIn, GateRef newIn)
{
    auto uses = Uses(oldIn);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        useIt = ReplaceIn(useIt, newIn);
    }
}

void GateAccessor::ReplaceIn(GateRef gate, size_t index, GateRef in)
{
    circuit_->ModifyIn(gate, index, in);
}

void GateAccessor::DeleteIn(GateRef gate, size_t idx)
{
    ASSERT(idx < circuit_->LoadGatePtrConst(gate)->GetNumIns());
    ASSERT(!circuit_->IsInGateNull(gate, idx));
    circuit_->LoadGatePtr(gate)->DeleteIn(idx);
}

void GateAccessor::ReplaceStateIn(GateRef gate, GateRef in, size_t index)
{
    ASSERT(index < GetStateCount(gate));
    circuit_->ModifyIn(gate, index, in);
}

void GateAccessor::ReplaceDependIn(GateRef gate, GateRef in, size_t index)
{
    ASSERT(index < GetDependCount(gate));
    size_t stateCount = GetStateCount(gate);
    circuit_->ModifyIn(gate, stateCount + index, in);
}

void GateAccessor::ReplaceValueIn(GateRef gate, GateRef in, size_t index)
{
    ASSERT(index < GetInValueCount(gate));
    size_t valueStartIndex = GetStateCount(gate) + GetDependCount(gate);
    circuit_->ModifyIn(gate, valueStartIndex + index, in);
}

void GateAccessor::DeleteGate(GateRef gate)
{
    circuit_->DeleteGate(gate);
}

MachineType GateAccessor::GetMachineType(GateRef gate) const
{
    return circuit_->GetMachineType(gate);
}

void GateAccessor::SetMachineType(GateRef gate, MachineType type)
{
    circuit_->SetMachineType(gate, type);
}

GateRef GateAccessor::GetConstantGate(MachineType bitValue, BitField bitfield, GateType type) const
{
    return circuit_->GetConstantGate(bitValue, bitfield, type);
}

bool GateAccessor::IsStateIn(const UseIterator &useIt) const
{
    size_t stateStartIndex = 0;
    size_t stateEndIndex = stateStartIndex + GetStateCount(*useIt);
    size_t index = useIt.GetIndex();
    return (index >= stateStartIndex && index < stateEndIndex);
}

bool GateAccessor::IsDependIn(const UseIterator &useIt) const
{
    size_t dependStartIndex = GetStateCount(*useIt);
    size_t dependEndIndex = dependStartIndex + GetDependCount(*useIt);
    size_t index = useIt.GetIndex();
    return (index >= dependStartIndex && index < dependEndIndex);
}

bool GateAccessor::IsValueIn(const UseIterator &useIt) const
{
    size_t valueStartIndex = GetStateCount(*useIt) + GetDependCount(*useIt);
    size_t valueEndIndex = valueStartIndex + GetInValueCount(*useIt);
    size_t index = useIt.GetIndex();
    return (index >= valueStartIndex && index < valueEndIndex);
}

bool GateAccessor::IsExceptionState(const UseIterator &useIt) const
{
    auto op = GetOpCode(*useIt);
    bool isDependSelector = (op == OpCode::DEPEND_SELECTOR) &&
                            (GetOpCode(GetIn(GetIn(*useIt, 0), useIt.GetIndex() - 1)) == OpCode::IF_EXCEPTION);
    bool isReturn = (op == OpCode::RETURN && GetOpCode(GetIn(*useIt, 0)) == OpCode::IF_EXCEPTION);
    return isDependSelector || isReturn;
}

bool GateAccessor::IsDependIn(GateRef gate, size_t index) const
{
    size_t dependStartIndex = GetStateCount(gate);
    size_t dependEndIndex = dependStartIndex + GetDependCount(gate);
    return (index >= dependStartIndex && index < dependEndIndex);
}

bool GateAccessor::IsValueIn(GateRef gate, size_t index) const
{
    size_t valueStartIndex = GetStateCount(gate) + GetDependCount(gate);
    size_t valueEndIndex = valueStartIndex + GetInValueCount(gate);
    return (index >= valueStartIndex && index < valueEndIndex);
}

void GateAccessor::DeleteGuardAndFrameState(GateRef gate)
{
    GateRef guard = GetDep(gate);
    if (GetOpCode(guard) == OpCode::GUARD) {
        GateRef dep = GetDep(guard);
        ReplaceDependIn(gate, dep);
        GateRef frameState = GetValueIn(guard, 1);
        DeleteGate(frameState);
        DeleteGate(guard);
    }
}

void GateAccessor::ReplaceGate(GateRef gate, GateRef state, GateRef depend, GateRef value)
{
    auto uses = Uses(gate);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        if (IsStateIn(useIt)) {
            useIt = ReplaceIn(useIt, state);
        } else if (IsDependIn(useIt)) {
            useIt = ReplaceIn(useIt, depend);
        } else if (IsValueIn(useIt)) {
            useIt = ReplaceIn(useIt, value);
        } else {
            UNREACHABLE();
        }
    }
    DeleteGate(gate);
}

GateType GateAccessor::GetLeftType(GateRef gate) const
{
    auto operandTypes = GetBitField(gate);
    auto temp = operandTypes >> CircuitBuilder::OPRAND_TYPE_BITS;
    return GateType(static_cast<uint32_t>(temp));
}

GateType GateAccessor::GetRightType(GateRef gate) const
{
    auto operandTypes = GetBitField(gate);
    auto temp = operandTypes >> CircuitBuilder::OPRAND_TYPE_BITS;
    return GateType(static_cast<uint32_t>(operandTypes ^ (temp << CircuitBuilder::OPRAND_TYPE_BITS)));
}
}  // namespace panda::ecmascript::kungfu
