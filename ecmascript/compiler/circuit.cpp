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

#include "ecmascript/base/mem_mmap.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/ecma_opcode_des.h"

namespace panda::ecmascript::kungfu {
Circuit::Circuit(bool isArch64) : space_(nullptr), circuitSize_(0), gateCount_(0), time_(1),
                                  dataSection_(), isArch64_(isArch64)
{
    space_ = panda::ecmascript::base::MemMmap::Mmap(CIRCUIT_SPACE, false);
    NewGate(OpCode(OpCode::CIRCUIT_ROOT), 0, {}, GateType::Empty());  // circuit root
    auto circuitRoot = Circuit::GetCircuitRoot(OpCode(OpCode::CIRCUIT_ROOT));
    NewGate(OpCode(OpCode::STATE_ENTRY), 0, {circuitRoot}, GateType::Empty());
    NewGate(OpCode(OpCode::DEPEND_ENTRY), 0, {circuitRoot}, GateType::Empty());
    NewGate(OpCode(OpCode::FRAMESTATE_ENTRY), 0, {circuitRoot}, GateType::Empty());
    NewGate(OpCode(OpCode::RETURN_LIST), 0, {circuitRoot}, GateType::Empty());
    NewGate(OpCode(OpCode::THROW_LIST), 0, {circuitRoot}, GateType::Empty());
    NewGate(OpCode(OpCode::CONSTANT_LIST), 0, {circuitRoot}, GateType::Empty());
    NewGate(OpCode(OpCode::ALLOCA_LIST), 0, {circuitRoot}, GateType::Empty());
    NewGate(OpCode(OpCode::ARG_LIST), 0, {circuitRoot}, GateType::Empty());
}

Circuit::~Circuit()
{
    panda::ecmascript::base::MemMmap::Munmap(space_, CIRCUIT_SPACE);
}

uint8_t *Circuit::AllocateSpace(size_t gateSize)
{
    circuitSize_ += gateSize;
    if (circuitSize_ > CIRCUIT_SPACE) {
        return nullptr;  // abort compilation
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return GetDataPtr(circuitSize_ - gateSize);
}

Gate *Circuit::AllocateGateSpace(size_t numIns)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>(AllocateSpace(Gate::GetGateSize(numIns)) + Gate::GetOutListSize(numIns));
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
GateRef Circuit::NewGate(OpCode opcode, MachineType bitValue, BitField bitfield, size_t numIns, const GateRef inList[],
                         GateType type, MarkCode mark)
{
#ifndef NDEBUG
    if (numIns != opcode.GetOpCodeNumIns(bitfield)) {
        LOG_COMPILER(ERROR) << "Invalid input list!"
                            << " op=" << opcode.Str() << " bitfield=" << bitfield
                            << " expected_num_in=" << opcode.GetOpCodeNumIns(bitfield) << " actual_num_in=" << numIns;
        UNREACHABLE();
    }
#endif
    std::vector<Gate *> inPtrList(numIns);
    auto gateSpace = AllocateGateSpace(numIns);
    for (size_t idx = 0; idx < numIns; idx++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        inPtrList[idx] = (inList[idx] == Circuit::NullGate()) ? nullptr : LoadGatePtr(inList[idx]);
    }
    ASSERT(opcode.GetMachineType() == MachineType::FLEX);
    auto newGate = new (gateSpace) Gate(gateCount_, opcode, bitValue, bitfield, inPtrList.data(), type, mark);
    gateCount_++;
    return GetGateRef(newGate);
}

GateRef Circuit::NewGate(OpCode opcode, MachineType bitValue, BitField bitfield, const std::vector<GateRef> &inList,
                         GateType type, MarkCode mark)
{
    return NewGate(opcode, bitValue, bitfield, inList.size(), inList.data(), type, mark);
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
GateRef Circuit::NewGate(OpCode opcode, BitField bitfield, size_t numIns, const GateRef inList[], GateType type,
                         MarkCode mark)
{
#ifndef NDEBUG
    if (numIns != opcode.GetOpCodeNumIns(bitfield)) {
        LOG_COMPILER(ERROR) << "Invalid input list!"
                            << " op=" << opcode.Str() << " bitfield=" << bitfield
                            << " expected_num_in=" << opcode.GetOpCodeNumIns(bitfield) << " actual_num_in=" << numIns;
        UNREACHABLE();
    }
#endif
    std::vector<Gate *> inPtrList(numIns);
    auto gateSpace = AllocateGateSpace(numIns);
    for (size_t idx = 0; idx < numIns; idx++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        inPtrList[idx] = (inList[idx] == Circuit::NullGate()) ? nullptr : LoadGatePtr(inList[idx]);
    }
    ASSERT(opcode.GetMachineType() != MachineType::FLEX);
    auto newGate = new (gateSpace) Gate(gateCount_, opcode, opcode.GetMachineType(), bitfield, inPtrList.data(), type,
                                        mark);
    gateCount_++;
    return GetGateRef(newGate);
}

GateRef Circuit::NewGate(OpCode opcode, BitField bitfield, const std::vector<GateRef> &inList, GateType type,
                         MarkCode mark)
{
    return NewGate(opcode, bitfield, inList.size(), inList.data(), type, mark);
}

void Circuit::PrintAllGates() const
{
    std::vector<GateRef> gateList;
    GetAllGates(gateList);
    for (const auto &gate : gateList) {
        LoadGatePtrConst(gate)->Print();
    }
}

void Circuit::PrintAllGatesWithBytecode() const
{
    std::vector<GateRef> gateList;
    GetAllGates(gateList);
    for (const auto &gate : gateList) {
        if (GetOpCode(gate) == OpCode::JS_BYTECODE) {
            BitField bitField = GetBitField(gate);
            auto opcode = GateBitFieldAccessor::GetByteCodeOpcode(bitField);
            std::string bytecodeStr = GetEcmaOpcodeStr(opcode);
            LoadGatePtrConst(gate)->PrintByteCode(bytecodeStr);
        } else {
            LoadGatePtrConst(gate)->Print();
        }
    }
}

void Circuit::GetAllGates(std::vector<GateRef>& gateList) const
{
    gateList.clear();
    gateList.push_back(0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (size_t out = sizeof(Gate); out < circuitSize_;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        out += Gate::GetGateSize(reinterpret_cast<const Out *>(LoadGatePtrConst(GateRef(out)))->GetIndex() + 1)) {
        gateList.push_back(GetGateRef(reinterpret_cast<const Out *>(LoadGatePtrConst(GateRef(out)))->GetGateConst()));
    }
}

GateRef Circuit::GetGateRef(const Gate *gate) const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<GateRef>(reinterpret_cast<const uint8_t *>(gate) - GetDataPtrConst(0));
}

Gate *Circuit::LoadGatePtr(GateRef shift)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>(GetDataPtr(shift));
}

const Gate *Circuit::LoadGatePtrConst(GateRef shift) const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Gate *>(GetDataPtrConst(shift));
}

GateRef Circuit::GetCircuitRoot(OpCode opcode)
{
    switch (opcode) {
        case OpCode::CIRCUIT_ROOT:
            return sizeof(In) * 0 + sizeof(Out) * 0 + sizeof(Gate) * 0;  // 0 0 0: offset of circuit root
        case OpCode::STATE_ENTRY:
            return sizeof(In) * 0 + sizeof(Out) * 1 + sizeof(Gate) * 1;  // 0 1 1: offset of state entry
        case OpCode::DEPEND_ENTRY:
            return sizeof(In) * 1 + sizeof(Out) * 2 + sizeof(Gate) * 2;  // 1 2 2: offset of depend entry
        case OpCode::FRAMESTATE_ENTRY:
            return sizeof(In) * 2 + sizeof(Out) * 3 + sizeof(Gate) * 3;  // 2 3 3: offset of framestate entry
        case OpCode::RETURN_LIST:
            return sizeof(In) * 3 + sizeof(Out) * 4 + sizeof(Gate) * 4;  // 3 4 4: offset of return list
        case OpCode::THROW_LIST:
            return sizeof(In) * 4 + sizeof(Out) * 5 + sizeof(Gate) * 5;  // 4 5 5: offset of throw list
        case OpCode::CONSTANT_LIST:
            return sizeof(In) * 5 + sizeof(Out) * 6 + sizeof(Gate) * 6;  // 5 6 6: offset of constant list
        case OpCode::ALLOCA_LIST:
            return sizeof(In) * 6 + sizeof(Out) * 7 + sizeof(Gate) * 7;  // 6 7 7: offset of alloca list
        case OpCode::ARG_LIST:
            return sizeof(In) * 7 + sizeof(Out) * 8 + sizeof(Gate) * 8;  // 7 8 8: offset of arg list
        default:
            UNREACHABLE();
    }
}

void Circuit::AdvanceTime() const
{
    auto &curTime = const_cast<TimeStamp &>(time_);
    curTime++;
    if (curTime == 0) {
        curTime = 1;
        ResetAllGateTimeStamps();
    }
}

void Circuit::ResetAllGateTimeStamps() const
{
    std::vector<GateRef> gateList;
    GetAllGates(gateList);
    for (auto &gate : gateList) {
        const_cast<Gate *>(LoadGatePtrConst(gate))->SetMark(MarkCode::NO_MARK, 0);
    }
}

TimeStamp Circuit::GetTime() const
{
    return time_;
}

MarkCode Circuit::GetMark(GateRef gate) const
{
    return LoadGatePtrConst(gate)->GetMark(GetTime());
}

void Circuit::SetMark(GateRef gate, MarkCode mark) const
{
    const_cast<Gate *>(LoadGatePtrConst(gate))->SetMark(mark, GetTime());
}

bool Circuit::Verify(GateRef gate) const
{
    return LoadGatePtrConst(gate)->Verify(IsArch64());
}

GateRef Circuit::NullGate()
{
    return Gate::InvalidGateRef;
}

bool Circuit::IsLoopHead(GateRef gate) const
{
    if (gate != NullGate()) {
        const Gate *curGate = LoadGatePtrConst(gate);
        return curGate->GetOpCode().IsLoopHead();
    }
    return false;
}

bool Circuit::IsControlCase(GateRef gate) const
{
    if (gate != NullGate()) {
        const Gate *curGate = LoadGatePtrConst(gate);
        return curGate->GetOpCode().IsControlCase();
    }
    return false;
}

bool Circuit::IsSelector(GateRef gate) const
{
    if (gate != NullGate()) {
        const Gate *curGate = LoadGatePtrConst(gate);
        return curGate->GetOpCode() == OpCode::VALUE_SELECTOR;
    }
    return false;
}

GateRef Circuit::GetIn(GateRef gate, size_t idx) const
{
    ASSERT(idx < LoadGatePtrConst(gate)->GetNumIns());
    if (IsInGateNull(gate, idx)) {
        return NullGate();
    }
    const Gate *curGate = LoadGatePtrConst(gate);
    return GetGateRef(curGate->GetInGateConst(idx));
}

bool Circuit::IsInGateNull(GateRef gate, size_t idx) const
{
    const Gate *curGate = LoadGatePtrConst(gate);
    return curGate->GetInConst(idx)->IsGateNull();
}

bool Circuit::IsFirstOutNull(GateRef gate) const
{
    const Gate *curGate = LoadGatePtrConst(gate);
    return curGate->IsFirstOutNull();
}

std::vector<GateRef> Circuit::GetOutVector(GateRef gate) const
{
    std::vector<GateRef> result;
    const Gate *curGate = LoadGatePtrConst(gate);
    if (!curGate->IsFirstOutNull()) {
        const Out *curOut = curGate->GetFirstOutConst();
        result.push_back(GetGateRef(curOut->GetGateConst()));
        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOutConst();
            result.push_back(GetGateRef(curOut->GetGateConst()));
        }
    }
    return result;
}

void Circuit::NewIn(GateRef gate, size_t idx, GateRef in)
{
#ifndef NDEBUG
    ASSERT(idx < LoadGatePtrConst(gate)->GetNumIns());
    ASSERT(Circuit::IsInGateNull(gate, idx));
#endif
    LoadGatePtr(gate)->NewIn(idx, LoadGatePtr(in));
}

void Circuit::ModifyIn(GateRef gate, size_t idx, GateRef in)
{
#ifndef NDEBUG
    ASSERT(idx < LoadGatePtrConst(gate)->GetNumIns());
    ASSERT(!Circuit::IsInGateNull(gate, idx));
#endif
    LoadGatePtr(gate)->ModifyIn(idx, LoadGatePtr(in));
}

void Circuit::DeleteIn(GateRef gate, size_t idx)
{
    ASSERT(idx < LoadGatePtrConst(gate)->GetNumIns());
    ASSERT(!Circuit::IsInGateNull(gate, idx));
    LoadGatePtr(gate)->DeleteIn(idx);
}

void Circuit::DeleteGate(GateRef gate)
{
    LoadGatePtr(gate)->DeleteGate();
}

void Circuit::DecreaseIn(GateRef gate, size_t idx)
{
    auto numIns = LoadGatePtrConst(gate)->GetNumIns();
    for (size_t i = idx; i < numIns - 1; i++) {
        ModifyIn(gate, i, GetIn(gate, i + 1));
    }
    DeleteIn(gate, numIns - 1);
    SetBitField(gate, GetBitField(gate) - 1);
}

void Circuit::SetOpCode(GateRef gate, OpCode opcode)
{
    LoadGatePtr(gate)->SetOpCode(opcode);
}

void Circuit::SetGateType(GateRef gate, GateType type)
{
    LoadGatePtr(gate)->SetGateType(type);
}

void Circuit::SetMachineType(GateRef gate, MachineType machineType)
{
    LoadGatePtr(gate)->SetMachineType(machineType);
}

GateType Circuit::GetGateType(GateRef gate) const
{
    return LoadGatePtrConst(gate)->GetGateType();
}

MachineType Circuit::GetMachineType(GateRef gate) const
{
    return LoadGatePtrConst(gate)->GetMachineType();
}

OpCode Circuit::GetOpCode(GateRef gate) const
{
    return LoadGatePtrConst(gate)->GetOpCode();
}

GateId Circuit::GetId(GateRef gate) const
{
    return LoadGatePtrConst(gate)->GetId();
}

BitField Circuit::GetBitField(GateRef gate) const
{
    return LoadGatePtrConst(gate)->GetBitField();
}

void Circuit::SetBitField(GateRef gate, BitField bitfield)
{
    LoadGatePtr(gate)->SetBitField(bitfield);
}

void Circuit::Print(GateRef gate) const
{
    LoadGatePtrConst(gate)->Print();
}

size_t Circuit::GetCircuitDataSize() const
{
    return circuitSize_;
}

const void *Circuit::GetSpaceDataStartPtrConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return GetDataPtrConst(0);
}

const void *Circuit::GetSpaceDataEndPtrConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return GetDataPtrConst(circuitSize_);
}

const uint8_t *Circuit::GetDataPtrConst(size_t offset) const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<uint8_t *>(space_) + offset;
}

uint8_t *Circuit::GetDataPtr(size_t offset)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return static_cast<uint8_t *>(space_) + offset;
}

panda::ecmascript::FrameType Circuit::GetFrameType() const
{
    return frameType_;
}

void Circuit::SetFrameType(panda::ecmascript::FrameType type)
{
    frameType_ = type;
}

GateRef Circuit::GetConstantGate(MachineType bitValue, BitField bitfield,
                                 GateType type)
{
    auto search = constantCache_.find({bitValue, bitfield, type});
    if (search != constantCache_.end()) {
        return constantCache_.at({bitValue, bitfield, type});
    }
    auto gate = NewGate(OpCode(OpCode::CONSTANT), bitValue, bitfield,
                        {GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))}, type);
    constantCache_[{bitValue, bitfield, type}] = gate;
    return gate;
}

GateRef Circuit::GetConstantDataGate(BitField bitfield, GateType type)
{
    auto search = constantDataCache_.find(bitfield);
    if (search != constantDataCache_.end()) {
        return constantDataCache_.at(bitfield);
    }
    auto gate = NewGate(OpCode(OpCode::CONST_DATA), bitfield,
                        {GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST))}, type);
    constantDataCache_[bitfield] = gate;
    return gate;
}

size_t Circuit::GetGateCount() const
{
    return gateCount_;
}
}  // namespace panda::ecmascript::kungfu
