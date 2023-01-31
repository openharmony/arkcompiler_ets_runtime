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

#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript::kungfu {
Circuit::Circuit(NativeAreaAllocator* allocator, bool isArch64) :
    circuitSize_(0), gateCount_(0), time_(1),
    isArch64_(isArch64), chunk_(allocator),
    root_(Circuit::NullGate()), metaBuilder_(chunk())
#ifndef NDEBUG
    , allGates_(chunk())
#endif
{
    space_ = panda::ecmascript::PageMap(CIRCUIT_SPACE, PAGE_PROT_READWRITE).GetMem();
    InitRoot();
}

Circuit::~Circuit()
{
    panda::ecmascript::PageUnmap(MemMap(space_, CIRCUIT_SPACE));
}

void Circuit::InitRoot()
{
    root_ = NewGate(metaBuilder_.CircuitRoot(), MachineType::NOVALUE, {}, GateType::Empty());
    NewGate(metaBuilder_.StateEntry(), MachineType::NOVALUE, { root_ }, GateType::Empty());
    NewGate(metaBuilder_.DependEntry(), MachineType::NOVALUE, { root_ }, GateType::Empty());
    NewGate(metaBuilder_.ReturnList(), MachineType::NOVALUE, { root_ }, GateType::Empty());
    NewGate(metaBuilder_.ArgList(), MachineType::NOVALUE, { root_ }, GateType::Empty());
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
GateRef Circuit::NewGate(const GateMetaData *meta, MachineType machineType,
    size_t numIns, const GateRef inList[], GateType type)
{
#ifndef NDEBUG
    if (numIns != meta->GetNumIns()) {
        LOG_COMPILER(ERROR) << "Invalid input list!"
                            << " op=" << meta->GetOpCode()
                            << " expected_num_in=" << meta->GetNumIns() << " actual_num_in=" << numIns;
        UNREACHABLE();
    }
#endif
    std::vector<Gate *> inPtrList(numIns);
    auto gateSpace = AllocateGateSpace(numIns);
    for (size_t idx = 0; idx < numIns; idx++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        inPtrList[idx] = (inList[idx] == Circuit::NullGate()) ? nullptr : LoadGatePtr(inList[idx]);
    }
    auto newGate = new (gateSpace) Gate(meta, gateCount_++, inPtrList.data(), machineType, type);
#ifndef NDEBUG
    allGates_.push_back(GetGateRef(newGate));
#endif
    return GetGateRef(newGate);
}

GateRef Circuit::NewGate(const GateMetaData *meta, const std::vector<GateRef> &inList)
{
    return NewGate(meta, MachineType::NOVALUE, inList.size(), inList.data(), GateType::Empty());
}

GateRef Circuit::NewGate(const GateMetaData *meta, MachineType machineType,
    const std::initializer_list<GateRef>& args, GateType type)
{
    return NewGate(meta, machineType, args.size(), args.begin(), type);
}

GateRef Circuit::NewGate(const GateMetaData *meta, MachineType machineType, GateType type)
{
    return NewGate(meta, machineType, {}, type);
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
            const Gate *gatePtr = LoadGatePtrConst(gate);
            auto opcode =  gatePtr->GetJSBytecodeMetaData()->GetByteCodeOpcode();
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
    for (size_t out = 0; out < circuitSize_;
        out += Gate::GetGateSize(reinterpret_cast<const Out *>(LoadGatePtrConst(GateRef(out)))->GetIndex() + 1)) {
        auto gatePtr = reinterpret_cast<const Out *>(LoadGatePtrConst(GateRef(out)))->GetGateConst();
        if (!gatePtr->GetMetaData()->IsNop()) {
            gateList.push_back(GetGateRef(gatePtr));
        }
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

void Circuit::Verify(GateRef gate) const
{
    LoadGatePtrConst(gate)->Verify(IsArch64());
}

GateRef Circuit::NullGate()
{
    return Gate::InvalidGateRef;
}

bool Circuit::IsLoopHead(GateRef gate) const
{
    if (gate != NullGate()) {
        const Gate *curGate = LoadGatePtrConst(gate);
        return curGate->GetMetaData()->IsLoopHead();
    }
    return false;
}

bool Circuit::IsControlCase(GateRef gate) const
{
    if (gate != NullGate()) {
        const Gate *curGate = LoadGatePtrConst(gate);
        return curGate->GetMetaData()->IsControlCase();
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
    ASSERT(!Circuit::IsInGateNull(gate, idx) || (GetOpCode(gate) == OpCode::SAVE_REGISTER));
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
    LoadGatePtr(gate)->SetMetaData(Nop());
}

void Circuit::DecreaseIn(GateRef gate, size_t idx)
{
    auto numIns = LoadGatePtrConst(gate)->GetNumIns();
    for (size_t i = idx; i < numIns - 1; i++) {
        ModifyIn(gate, i, GetIn(gate, i + 1));
    }
    DeleteIn(gate, numIns - 1);
    GateMetaData *meta = const_cast<GateMetaData *>(
            LoadGatePtr(gate)->GetMetaData());
    if (meta->GetKind() == GateMetaData::Kind::MUTABLE_WITH_SIZE) {
        meta->DecreaseIn(idx);
    } else {
        meta = metaBuilder_.NewGateMetaData(meta);
        meta->DecreaseIn(idx);
        LoadGatePtr(gate)->SetMetaData(meta);
    }
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

GateRef Circuit::GetConstantGate(MachineType machineType, uint64_t value,
                                 GateType type)
{
    auto search = constantCache_.find({machineType, value, type});
    if (search != constantCache_.end()) {
        return constantCache_.at({machineType, value, type});
    }
    auto gate = NewGate(metaBuilder_.Constant(value), machineType, type);
    constantCache_[{machineType, value, type}] = gate;
    return gate;
}

GateRef Circuit::NewArg(MachineType machineType, size_t index,
                        GateType type, GateRef argRoot)
{
    return NewGate(metaBuilder_.Arg(index), machineType, { argRoot }, type);
}

GateRef Circuit::GetConstantDataGate(uint64_t value, GateType type)
{
    auto search = constantDataCache_.find(value);
    if (search != constantDataCache_.end()) {
        return constantDataCache_.at(value);
    }
    auto gate = NewGate(metaBuilder_.ConstData(value), MachineType::ARCH, type);
    constantDataCache_[value] = gate;
    return gate;
}

size_t Circuit::GetGateCount() const
{
    return gateCount_;
}

GateRef Circuit::GetStateRoot() const
{
    const GateAccessor acc(const_cast<Circuit*>(this));
    return acc.GetStateRoot();
}

GateRef Circuit::GetDependRoot() const
{
    const GateAccessor acc(const_cast<Circuit*>(this));
    return acc.GetDependRoot();
}

GateRef Circuit::GetArgRoot() const
{
    const GateAccessor acc(const_cast<Circuit*>(this));
    return acc.GetArgRoot();
}

GateRef Circuit::GetReturnRoot() const
{
    const GateAccessor acc(const_cast<Circuit*>(this));
    return acc.GetReturnRoot();
}
}  // namespace panda::ecmascript::kungfu
