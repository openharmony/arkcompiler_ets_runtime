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

#include "ecmascript/compiler/gate.h"
#include <iostream>
#include <sstream>

namespace panda::ecmascript::kungfu {
void Gate::CheckNullInput() const
{
    const auto numIns = GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        if (IsInGateNull(idx)) {
            CheckFailed("In list contains null", idx);
        }
    }
}

void Gate::CheckFailed(std::string errorString, size_t highlightIdx) const
{
    LOG_COMPILER(ERROR) << "[Verifier][Error] Gate level input list schema verify failed";
    Print("", true, highlightIdx);
    LOG_COMPILER(FATAL) << "Note: " << errorString;
}

void Gate::CheckInputOpcode(size_t idx, OpCode expected) const
{
    OpCode actual = GetInGateConst(idx)->GetOpCode();
    if (actual != expected) {
        CheckFailed("State input does not match (expected:" + GateMetaData::Str(expected) +
                    " actual:" + GateMetaData::Str(actual) + ")", idx);
    }
}

void Gate::CheckInputMachineType(size_t idx, MachineType expected, bool isArch64) const
{
    MachineType actual = GetInGateConst(idx)->GetMachineType();
    if (expected == MachineType::FLEX) {
        expected = GetMachineType();
    }
    if (expected == MachineType::ARCH) {
        expected = isArch64 ? MachineType::I64 : MachineType::I32;
    }
    if (actual == MachineType::ARCH) {
        actual = isArch64 ? MachineType::I64 : MachineType::I32;
    }
    if (actual != expected) {
        CheckFailed("Value input does not match (expected:" +
                    MachineTypeToStr(expected) + " actual:" + MachineTypeToStr(actual) + ")", idx);
    }
}

void Gate::CheckNotInputMachineType(size_t idx, MachineType notExpected) const
{
    MachineType actual = GetInGateConst(idx)->GetMachineType();
    if (actual == notExpected) {
        CheckFailed("Value input does not match (notExpected:" +
                    MachineTypeToStr(notExpected) + " actual:" + MachineTypeToStr(actual) + ")", idx);
    }
}

void Gate::CheckGeneralState(size_t idx) const
{
    auto gatePtr = GetInGateConst(idx);
    OpCode actual = gatePtr->GetOpCode();
    if (!gatePtr->meta_->IsGeneralState()) {
        CheckFailed("State input does not match (expected:<General State> actual:" +
                    GateMetaData::Str(actual) + ")", idx);
    }
}

void Gate::CheckState(size_t idx) const
{
    auto gatePtr = GetInGateConst(idx);
    OpCode actual = gatePtr->GetOpCode();
    if ((actual != OpCode::STATE_ENTRY) && (!gatePtr->meta_->IsState())) {
        CheckFailed("State input does not match (expected:<State> actual:" +
                    GateMetaData::Str(actual) + ")", idx);
    }
}

void Gate::CheckStateInput() const
{
    size_t stateStart = 0;
    size_t stateEnd = GetStateCount();
    for (size_t idx = stateStart; idx < stateEnd; idx++) {
        bool needCheck = true;
        switch (GetOpCode()) {
            case OpCode::IF_TRUE:
            case OpCode::IF_FALSE:
                ASSERT(idx == stateStart);
                CheckInputOpcode(idx, OpCode::IF_BRANCH);
                needCheck = false;
                break;
            case OpCode::SWITCH_CASE:
            case OpCode::DEFAULT_CASE:
                ASSERT(idx == stateStart);
                CheckInputOpcode(idx, OpCode::SWITCH_BRANCH);
                needCheck = false;
                break;
            default:
                break;
        }
        if (needCheck) {
            CheckState(idx);
        }
    }
}

void Gate::CheckValueInput(bool isArch64) const
{
    size_t valueStart = GetInValueStarts();
    size_t valueEnd = valueStart + GetInValueCount();
    for (size_t idx = valueStart; idx < valueEnd; idx++) {
        switch (GetOpCode()) {
            case OpCode::IF_BRANCH:
                ASSERT(idx == valueStart);
                CheckInputMachineType(idx, MachineType::I1, isArch64);
                break;
            case OpCode::VALUE_SELECTOR:
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::EXP:
            case OpCode::SDIV:
            case OpCode::SMOD:
            case OpCode::UDIV:
            case OpCode::UMOD:
            case OpCode::FDIV:
            case OpCode::FMOD:
            case OpCode::AND:
            case OpCode::XOR:
            case OpCode::OR:
            case OpCode::LSL:
            case OpCode::LSR:
            case OpCode::ASR:
                CheckInputMachineType(idx, MachineType::FLEX, isArch64);
                break;
            case OpCode::REV:
                ASSERT(idx == valueStart);
                CheckInputMachineType(idx, MachineType::I1, isArch64);
                break;
            case OpCode::LOAD:
                ASSERT(idx == valueStart);
                CheckInputMachineType(idx, MachineType::ARCH, isArch64);
                break;
            case OpCode::STORE:
                if ((idx == valueStart + 1) || (idx == valueStart + 2)) { // 1:base, 2:offset
                    CheckInputMachineType(idx, MachineType::ARCH, isArch64);
                }
                break;
            case OpCode::STORE_WITHOUT_BARRIER:
                if (idx == valueStart) {
                    CheckInputMachineType(idx, MachineType::ARCH, isArch64);
                }
                break;
            case OpCode::HEAP_ALLOC: {
                if (idx == valueStart + 1) { // 1: size offset
                    CheckInputMachineType(idx, MachineType::I64, isArch64);
                }
                break;
            }
            case OpCode::TAGGED_TO_INT64:
            case OpCode::INT64_TO_TAGGED:
                ASSERT(idx == valueStart);
                CheckInputMachineType(valueStart, MachineType::I64, isArch64);
                break;
            case OpCode::OBJECT_TYPE_CHECK:
            case OpCode::LOAD_ELEMENT:
            case OpCode::STORE_ELEMENT:
                if (idx == valueStart) { // 1: idx 1
                    CheckInputMachineType(idx, MachineType::I64, isArch64);
                }
                break;
            case OpCode::FCMP:
                CheckInputMachineType(idx, MachineType::F64, isArch64);
                break;
            case OpCode::ICMP:
                CheckNotInputMachineType(idx, MachineType::F64);
                break;
            default:
                break;
        }
    }
}

void Gate::CheckDependInput() const
{
    size_t dependStart = GetStateCount();
    size_t dependEnd = dependStart + GetDependCount();
    for (size_t idx = dependStart; idx < dependEnd; idx++) {
        if (GetInGateConst(idx)->GetDependCount() == 0 &&
            GetInGateConst(idx)->GetOpCode() != OpCode::DEPEND_ENTRY) {
            LOG_COMPILER(ERROR) << "depend in of " << GetId() << GateMetaData::Str(GetOpCode()) << "is "
                << GetInGateConst(idx)->GetId() << GateMetaData::Str(GetInGateConst(idx)->GetOpCode());
            CheckFailed("Depend input is side-effect free", idx);
        }
    }
}

void Gate::CheckRootInput() const
{
    size_t rootStart = GetInValueStarts() + GetInValueCount();
    if (meta_->HasRoot()) {
        switch (GetOpCode()) {
            case OpCode::STATE_ENTRY:
            case OpCode::DEPEND_ENTRY:
            case OpCode::RETURN_LIST:
            case OpCode::ARG_LIST:
                CheckInputOpcode(rootStart, OpCode::CIRCUIT_ROOT);
                break;
            case OpCode::ARG:
                CheckInputOpcode(rootStart, OpCode::ARG_LIST);
                break;
            case OpCode::RETURN:
            case OpCode::RETURN_VOID:
                CheckInputOpcode(rootStart, OpCode::RETURN_LIST);
                break;
            default:
                break;
        }
    }
}

void Gate::CheckFrameStateInput() const
{
    size_t frameStateStart = GetInFrameStateStarts();
    if (meta_->HasFrameState()) {
        CheckInputOpcode(frameStateStart, OpCode::FRAME_STATE);
    }
}

std::string Gate::GetValueInAndOut(bool inListPreview, size_t highlightIdx) const
{
    auto opcode = GetOpCode();
    if (opcode != OpCode::NOP && opcode != OpCode::DEAD) {
        std::ostringstream log("{\"id\":");
        log << std::to_string(id_) << ", \"op\":\"" << GateMetaData::Str(opcode) << "\", ";
        log << "\",\"in\":[";
        size_t idx = 0;
        auto stateSize = GetStateCount();
        auto dependSize = GetDependCount();
        auto valueSize = GetInValueCount();
        auto frameStateSize = GetInFrameStateCount();
        auto rootSize = GetRootCount();
        size_t start = 0;
        size_t end = stateSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += dependSize;
        start += stateSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += valueSize;
        start += dependSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += frameStateSize;
        start += valueSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += rootSize;
        start += frameStateSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log, true);

        log << "], \"out\":[";

        if (!IsFirstOutNull()) {
            const Out *curOut = GetFirstOutConst();
            opcode = curOut->GetGateConst()->GetOpCode();
            log << std::to_string(curOut->GetGateConst()->GetId()) +
                    (inListPreview ? std::string(":" + GateMetaData::Str(opcode)) : std::string(""));

            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                log << ", " << std::to_string(curOut->GetGateConst()->GetId()) <<
                       (inListPreview ? std::string(":" + GateMetaData::Str(opcode))
                                       : std::string(""));
            }
        }
        log << "]},";
        return log.str();
    }
    return "";
}

void Gate::CheckStateOutput(const std::string& methodName) const
{
    if (!GetMetaData()->IsState()) {
        return;
    }
    size_t cnt = 0;
    const Gate *curGate = this;
    if (!curGate->IsFirstOutNull()) {
        const Out *curOut = curGate->GetFirstOutConst();
        auto meta = curOut->GetGateConst()->GetMetaData();
        if (curOut->IsStateEdge() && meta->IsState()) {
            cnt++;
        }
        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOutConst();
            meta = curOut->GetGateConst()->GetMetaData();
            if (curOut->IsStateEdge() && meta->IsState()) {
                cnt++;
            }
        }
    }
    size_t expected = 0;
    bool needCheck = true;
    if (GetMetaData()->IsTerminalState()) {
        expected = 0;
    } else if (GetOpCode() == OpCode::IF_BRANCH || GetOpCode() == OpCode::JS_BYTECODE) {
        expected = 2; // 2: expected number of state out branches
    } else if (GetOpCode() == OpCode::SWITCH_BRANCH) {
        needCheck = false;
    } else {
        expected = 1;
    }
    if (needCheck && cnt != expected) {
        curGate->Print();
        std::string log = curGate->GetValueInAndOut(true);
        CheckFailed("Number of state out branches is not valid (expected:" + std::to_string(expected) +
            " actual:" + std::to_string(cnt) + ") methodName:" + methodName + " gateValue:" + log, -1);
    }
}

void Gate::CheckBranchOutput() const
{
    std::map<std::pair<OpCode, BitField>, size_t> setOfOps;
    if (GetOpCode() == OpCode::IF_BRANCH) {
        size_t cnt = 0;
        const Gate *curGate = this;
        if (!curGate->IsFirstOutNull()) {
            const Out *curOut = curGate->GetFirstOutConst();
            if (curOut->GetGateConst()->GetMetaData()->IsState() && curOut->IsStateEdge()) {
                ASSERT(!curOut->GetGateConst()->GetMetaData()->IsFixed());
                setOfOps[{curOut->GetGateConst()->GetOpCode(), curOut->GetGateConst()->GetStateCount()}]++;
                cnt++;
            }
            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                if (curOut->GetGateConst()->GetMetaData()->IsState() && curOut->IsStateEdge()) {
                    ASSERT(!curOut->GetGateConst()->GetMetaData()->IsFixed());
                    setOfOps[{curOut->GetGateConst()->GetOpCode(), curOut->GetGateConst()->GetStateCount()}]++;
                    cnt++;
                }
            }
        }
        if (setOfOps.size() != cnt) {
            CheckFailed("Duplicate state out branches", -1);
        }
    }
}

void Gate::CheckNOP() const
{
    if (GetOpCode() == OpCode::NOP || GetOpCode() == OpCode::DEAD) {
        if (!IsFirstOutNull()) {
            CheckFailed("NOP gate used by other gates", -1);
        }
    }
}

void Gate::CheckSelector() const
{
    if (GetOpCode() == OpCode::VALUE_SELECTOR || GetOpCode() == OpCode::DEPEND_SELECTOR) {
        auto stateOp = GetInGateConst(0)->GetOpCode();
        if (stateOp == OpCode::MERGE || stateOp == OpCode::LOOP_BEGIN) {
            ASSERT(GetNumIns() > 0);
            if (GetInGateConst(0)->GetNumIns() != GetNumIns() - 1) {
                if (GetOpCode() == OpCode::DEPEND_SELECTOR) {
                    CheckFailed("Number of depend flows does not match control flows (expected:" +
                            std::to_string(GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(GetNumIns() - 1) + ")",
                        -1);
                } else {
                    CheckFailed("Number of data flows does not match control flows (expected:" +
                            std::to_string(GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(GetNumIns() - 1) + ")",
                        -1);
                }
            }
        } else {
            CheckFailed(
                "State input does not match (expected:[MERGE|LOOP_BEGIN] actual:" +
                GateMetaData::Str(stateOp) + ")", 0);
        }
    }
}

void Gate::CheckRelay() const
{
    if (GetOpCode() == OpCode::DEPEND_RELAY) {
        auto stateOp = GetInGateConst(0)->GetOpCode();
        switch (stateOp) {
            case OpCode::IF_TRUE:
            case OpCode::IF_FALSE:
            case OpCode::SWITCH_CASE:
            case OpCode::DEFAULT_CASE:
            case OpCode::IF_SUCCESS:
            case OpCode::IF_EXCEPTION:
            case OpCode::ORDINARY_BLOCK:
            case OpCode::DEOPT_CHECK:
                break;
            default:
                CheckFailed("State input does not match ("
                    "expected:[IF_TRUE|IF_FALSE|SWITCH_CASE|DEFAULT_CASE|"
                    "IF_SUCCESS|IF_EXCEPTION|ORDINARY_BLOCK|DEOPT_CHECK] actual:" +
                    GateMetaData::Str(stateOp) + ")", 0);
                break;
        }
    }
}

void Gate::Verify(bool isArch64, const std::string& methodName) const
{
    CheckNullInput();
    CheckStateInput();
    CheckValueInput(isArch64);
    CheckDependInput();
    CheckFrameStateInput();
    CheckRootInput();
    CheckStateOutput(methodName);
    CheckBranchOutput();
    CheckNOP();
    CheckSelector();
    CheckRelay();
}

void Out::SetNextOut(const Out *ptr)
{
    nextOut_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Out *Out::GetNextOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + nextOut_);
}

const Out *Out::GetNextOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + nextOut_);
}

void Out::SetPrevOut(const Out *ptr)
{
    prevOut_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Out *Out::GetPrevOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + prevOut_);
}

const Out *Out::GetPrevOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + prevOut_);
}

void Out::SetIndex(OutIdx idx)
{
    idx_ = idx;
}

OutIdx Out::GetIndex() const
{
    return idx_;
}

Gate *Out::GetGate()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>(&this[idx_ + 1]);
}

const Gate *Out::GetGateConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Gate *>(&this[idx_ + 1]);
}

void Out::SetPrevOutNull()
{
    prevOut_ = 0;
}

bool Out::IsPrevOutNull() const
{
    return prevOut_ == 0;
}

void Out::SetNextOutNull()
{
    nextOut_ = 0;
}

bool Out::IsNextOutNull() const
{
    return nextOut_ == 0;
}

bool Out::IsStateEdge() const
{
    return idx_ < GetGateConst()->GetStateCount();
}

void In::SetGate(const Gate *ptr)
{
    gatePtr_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>((reinterpret_cast<const uint8_t *>(ptr)) - (reinterpret_cast<const uint8_t *>(this)));
}

Gate *In::GetGate()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Gate *>((reinterpret_cast<uint8_t *>(this)) + gatePtr_);
}

const Gate *In::GetGateConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Gate *>((reinterpret_cast<const uint8_t *>(this)) + gatePtr_);
}

void In::SetGateNull()
{
    gatePtr_ = Gate::InvalidGateRef;
}

bool In::IsGateNull() const
{
    return gatePtr_ == Gate::InvalidGateRef;
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
Gate::Gate(const GateMetaData* meta, GateId id, Gate *inList[], MachineType machineType, GateType type)
    : meta_(meta), id_(id), type_(type), machineType_(machineType)
{
    auto numIns = GetNumIns();
    if (numIns == 0) {
        auto curOut = GetOut(0);
        curOut->SetIndex(0);
        return;
    }
    for (size_t idx = 0; idx < numIns; idx++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto in = inList[idx];
        if (in == nullptr) {
            GetIn(idx)->SetGateNull();
        } else {
            NewIn(idx, in);
        }
        auto curOut = GetOut(idx);
        curOut->SetIndex(idx);
    }
}

void Gate::NewIn(size_t idx, Gate *in)
{
    GetIn(idx)->SetGate(in);
    auto curOut = GetOut(idx);
    if (in->IsFirstOutNull()) {
        curOut->SetNextOutNull();
    } else {
        curOut->SetNextOut(in->GetFirstOut());
        in->GetFirstOut()->SetPrevOut(curOut);
    }
    curOut->SetPrevOutNull();
    in->SetFirstOut(curOut);
}

void Gate::ModifyIn(size_t idx, Gate *in)
{
    DeleteIn(idx);
    NewIn(idx, in);
}

void Gate::DeleteIn(size_t idx)
{
    if (!GetOut(idx)->IsNextOutNull() && !GetOut(idx)->IsPrevOutNull()) {
        GetOut(idx)->GetPrevOut()->SetNextOut(GetOut(idx)->GetNextOut());
        GetOut(idx)->GetNextOut()->SetPrevOut(GetOut(idx)->GetPrevOut());
    } else if (GetOut(idx)->IsNextOutNull() && !GetOut(idx)->IsPrevOutNull()) {
        GetOut(idx)->GetPrevOut()->SetNextOutNull();
    } else if (!GetOut(idx)->IsNextOutNull()) {  // then GetOut(idx)->IsPrevOutNull() is true
        GetIn(idx)->GetGate()->SetFirstOut(GetOut(idx)->GetNextOut());
        GetOut(idx)->GetNextOut()->SetPrevOutNull();
    } else {  // only this out now
        GetIn(idx)->GetGate()->SetFirstOutNull();
    }
    GetIn(idx)->SetGateNull();
}

void Gate::DeleteGate()
{
    auto numIns = GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        DeleteIn(idx);
    }
}

Out *Gate::GetOut(size_t idx)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<Out *>(this)[-1 - idx];
}

const Out *Gate::GetOutConst(size_t idx) const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<const Out *>(this)[-1 - idx];
}

Out *Gate::GetFirstOut()
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<Out *>((reinterpret_cast<uint8_t *>(this)) + firstOut_);
}

const Out *Gate::GetFirstOutConst() const
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return reinterpret_cast<const Out *>((reinterpret_cast<const uint8_t *>(this)) + firstOut_);
}

void Gate::SetFirstOutNull()
{
    firstOut_ = 0;
}

bool Gate::IsFirstOutNull() const
{
    return firstOut_ == 0;
}

void Gate::SetFirstOut(const Out *firstOut)
{
    firstOut_ =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        static_cast<GateRef>(reinterpret_cast<const uint8_t *>(firstOut) - reinterpret_cast<const uint8_t *>(this));
}

In *Gate::GetIn(size_t idx)
{
#ifndef NDEBUG
    if (idx >= GetNumIns()) {
        LOG_COMPILER(INFO) << std::dec << "Gate In access out-of-bound! (idx=" << idx << ")";
        Print();
        ASSERT(false);
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<In *>(this + 1)[idx];
}

const In *Gate::GetInConst(size_t idx) const
{
#ifndef NDEBUG
    if (idx >= GetNumIns()) {
        LOG_COMPILER(INFO) << std::dec << "Gate In access out-of-bound! (idx=" << idx << ")";
        Print();
        ASSERT(false);
    }
#endif
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return &reinterpret_cast<const In *>(this + 1)[idx];
}

Gate *Gate::GetInGate(size_t idx)
{
    return GetIn(idx)->GetGate();
}

const Gate *Gate::GetInGateConst(size_t idx) const
{
    return GetInConst(idx)->GetGateConst();
}

bool Gate::IsInGateNull(size_t idx) const
{
    return GetInConst(idx)->IsGateNull();
}

GateId Gate::GetId() const
{
    return id_;
}

OpCode Gate::GetOpCode() const
{
    return meta_->GetOpCode();
}

size_t Gate::GetNumIns() const
{
    return meta_->GetNumIns();
}

size_t Gate::GetInValueStarts() const
{
    return meta_->GetInValueStarts();
}

size_t Gate::GetInFrameStateStarts() const
{
    return meta_->GetInFrameStateStarts();
}

size_t Gate::GetStateCount() const
{
    return meta_->GetStateCount();
}

size_t Gate::GetDependCount() const
{
    return meta_->GetDependCount();
}

size_t Gate::GetInValueCount() const
{
    return meta_->GetInValueCount();
}

size_t Gate::GetInFrameStateCount() const
{
    return meta_->GetInFrameStateCount();
}

size_t Gate::GetRootCount() const
{
    return meta_->GetRootCount();
}

std::string Gate::MachineTypeStr(MachineType machineType) const
{
    const std::map<MachineType, const char *> strMap = {
        {NOVALUE, "NOVALUE"},
        {ANYVALUE, "ANYVALUE"},
        {ARCH, "ARCH"},
        {FLEX, "FLEX"},
        {I1, "I1"},
        {I8, "I8"},
        {I16, "I16"},
        {I32, "I32"},
        {I64, "I64"},
        {F32, "F32"},
        {F64, "F64"},
    };
    if (strMap.count(machineType) > 0) {
        return strMap.at(machineType);
    }
    return "MachineType-" + std::to_string(machineType);
}

std::string Gate::GateTypeStr(GateType gateType) const
{
    static const std::map<GateType, const char *> strMap = {
        {GateType::NJSValue(), "NJS_VALUE"},
        {GateType::TaggedValue(), "TAGGED_VALUE"},
        {GateType::TaggedPointer(), "TAGGED_POINTER"},
        {GateType::TaggedNPointer(), "TAGGED_NPOINTER"},
        {GateType::Empty(), "EMPTY"},
        {GateType::AnyType(), "ANY_TYPE"},
    };

    std::string name = "";
    if (strMap.count(gateType) > 0) {
        name = strMap.at(gateType);
    }
    uint32_t r = gateType.GetType();
    return name + std::string("-gateType(") + std::to_string(r) + std::string(")");
}

void Gate::Print(std::string additionOp, bool inListPreview, size_t highlightIdx, std::string_view comment) const
{
    LOG_COMPILER(INFO) << ToString(additionOp, inListPreview, highlightIdx, comment);
}

void Gate::DumpHeader(std::ostringstream &oss, const std::string& additionOp) const
{
    auto opcode = GetOpCode();
    ASSERT(opcode != OpCode::NOP && opcode != OpCode::DEAD);

    oss << "{\"id\":" << std::to_string(id_) << ", \"op\":\"" << GateMetaData::Str(opcode) << "\", ";
    if (additionOp.compare("") != 0) {
        auto additionOpName = (opcode == OpCode::JS_BYTECODE) ? "bytecode" : "typedop";
        oss << "\"" << additionOpName << "\":\"" << additionOp;
        oss << "\", ";
    }
    oss << "\"MType\":\"" << MachineTypeStr(GetMachineType()) << ", ";

    oss << "bitfield=0x" << std::hex << TryGetValue() << std::dec << ", ";
    oss << "type=" << GateTypeStr(type_) << ", ";
    oss << "stamp=" << std::to_string(static_cast<uint32_t>(stamp_)) << ", ";
    oss << "mark=" << std::to_string(static_cast<uint32_t>(mark_)) << ", ";
}

void Gate::DumpInputs(std::ostringstream &oss, bool inListPreview, size_t highlightIdx) const
{
    [[maybe_unused]] auto opcode = GetOpCode();
    ASSERT(opcode != OpCode::NOP && opcode != OpCode::DEAD);

    size_t idx = 0;
    auto stateSize = GetStateCount();
    auto dependSize = GetDependCount();
    auto valueSize = GetInValueCount();
    auto frameStateSize = GetInFrameStateCount();
    auto rootSize = GetRootCount();
    size_t start = 0;
    size_t end = stateSize;

    oss << "\",\"in\":[";
    idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, oss);
    end += dependSize;
    start += stateSize;
    idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, oss);
    end += valueSize;
    start += dependSize;
    idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, oss);
    end += frameStateSize;
    start += valueSize;
    idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, oss);
    end += rootSize;
    start += frameStateSize;
    idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, oss, true);
    oss << "]";
}

void Gate::DumpOutputs(std::ostringstream &oss, bool inListPreview) const
{
    auto opcode = GetOpCode();
    ASSERT(opcode != OpCode::NOP && opcode != OpCode::DEAD);

    oss << ", \"out\":[";
    if (!IsFirstOutNull()) {
        const Out *curOut = GetFirstOutConst();
        opcode = curOut->GetGateConst()->GetOpCode();
        oss << std::to_string(curOut->GetGateConst()->GetId()) +
                (inListPreview ? std::string(":" + GateMetaData::Str(opcode)) : std::string(""));

        while (!curOut->IsNextOutNull()) {
            curOut = curOut->GetNextOutConst();
            oss << ", " +  std::to_string(curOut->GetGateConst()->GetId()) +
                    (inListPreview ? std::string(":" + GateMetaData::Str(opcode))
                                    : std::string(""));
        }
    }
    oss << "]";
}

static void DumpComment(std::ostringstream &oss, std::string_view comment)
{
    oss << ", \"comment\":\"" << comment << "\"";
}

std::string Gate::ToString(std::string additionOp, bool inListPreview, size_t highlightIdx,
    std::string_view comment) const
{
    auto opcode = GetOpCode();
    if (opcode == OpCode::NOP || opcode == OpCode::DEAD) {
        return "";
    }

    std::ostringstream oss;
    oss << std::dec;
    DumpHeader(oss, additionOp);
    DumpInputs(oss, inListPreview, highlightIdx);
    DumpOutputs(oss, inListPreview);
    if (!comment.empty()) {
        DumpComment(oss, comment);
    }
    oss << "},";
    return oss.str();
}

void Gate::ShortPrint(std::string bytecode, bool inListPreview, size_t highlightIdx) const
{
    auto opcode = GetOpCode();
    if (opcode != OpCode::NOP && opcode != OpCode::DEAD) {
        std::ostringstream log;
        log << "(\"id\"=" << std::to_string(id_) << ", \"op\"=\"" << GateMetaData::Str(opcode) << "\", ";
        log << ((bytecode.compare("") == 0) ? "" : "bytecode=") << bytecode;
        log << ((bytecode.compare("") == 0) ? "" : ", ");
        log << "\"MType\"=\"" + MachineTypeStr(GetMachineType()) + ", ";
        log << "bitfield=0x" << std::hex << TryGetValue() << std::dec << ", ";
        log << "type=" + GateTypeStr(type_) + ", ";
        log << "\", in=[";

        size_t idx = 0;
        auto stateSize = GetStateCount();
        auto dependSize = GetDependCount();
        auto valueSize = GetInValueCount();
        auto frameStateSize = GetInFrameStateCount();
        auto rootSize = GetRootCount();
        size_t start = 0;
        size_t end = stateSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += dependSize;
        start += stateSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += valueSize;
        start += dependSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += frameStateSize;
        start += valueSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log);
        end += rootSize;
        start += frameStateSize;
        idx = PrintInGate(end, idx, start, inListPreview, highlightIdx, log, true);

        log << "], out=[";

        if (!IsFirstOutNull()) {
            const Out *curOut = GetFirstOutConst();
            opcode = curOut->GetGateConst()->GetOpCode();
            log << std::to_string(curOut->GetGateConst()->GetId()) <<
                   (inListPreview ? std::string(":" + GateMetaData::Str(opcode)) : std::string(""));

            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                log << ", " <<  std::to_string(curOut->GetGateConst()->GetId()) <<
                       (inListPreview ? std::string(":" + GateMetaData::Str(opcode))
                                      : std::string(""));
            }
        }
        log << "])";
        LOG_COMPILER(INFO) << std::dec << log.str();
    }
}

size_t Gate::PrintInGate(size_t numIns, size_t idx, size_t size, bool inListPreview, size_t highlightIdx,
                         std::ostringstream &log, bool isEnd) const
{
    log << "[";
    for (; idx < numIns; idx++) {
        log << ((idx == size) ? "" : ", ");
        log << ((idx == highlightIdx) ? "\033[4;31m" : "");
        log << ((IsInGateNull(idx)
                ? "N"
                : (std::to_string(GetInGateConst(idx)->GetId()) +
                    (inListPreview ? std::string(":" + GateMetaData::Str(GetInGateConst(idx)->GetOpCode()))
                                   : std::string("")))));
        log << ((idx == highlightIdx) ? "\033[0m" : "");
    }
    log << "]";
    log << ((isEnd) ? "" : ", ");
    return idx;
}

std::string Gate::GetBytecodeStr() const
{
    switch (GetOpCode()) {
        case OpCode::JS_BYTECODE: {
            return GetJSBytecodeMetaData()->Str();
        }
        case OpCode::TYPED_BINARY_OP: {
            auto typedOp = TypedBinaryAccessor(GetOneParameterMetaData()->GetValue()).GetTypedBinOp();
            return GateMetaData::Str(typedOp);
        }
        case OpCode::TYPED_UNARY_OP: {
            auto typedOp = TypedUnaryAccessor(GetOneParameterMetaData()->GetValue()).GetTypedUnOp();
            return GateMetaData::Str(typedOp);
        }
        case OpCode::TYPED_CONDITION_JUMP: {
            auto typedOp = TypedJumpAccessor(GetOneParameterMetaData()->GetValue()).GetTypedJumpOp();
            return GateMetaData::Str(typedOp);
        }
        case OpCode::LOAD_ELEMENT: {
            auto typedOp = static_cast<TypedLoadOp>(GetOneParameterMetaData()->GetValue());
            return GateMetaData::Str(typedOp);
        }
        case OpCode::STORE_ELEMENT: {
            auto typedOp = static_cast<TypedStoreOp>(GetOneParameterMetaData()->GetValue());
            return GateMetaData::Str(typedOp);
        }
        case OpCode::TYPED_CALLTARGETCHECK_OP: {
            TypedCallTargetCheckAccessor accessor(GetOneParameterMetaData()->GetValue());
            auto typedOp = accessor.GetCallTargetCheckOp();
            return GateMetaData::Str(typedOp);
        }
        case OpCode::CONVERT:
        case OpCode::CHECK_AND_CONVERT: {
            ValuePairTypeAccessor accessor(GetOneParameterMetaData()->GetValue());
            return GateMetaData::Str(accessor.GetSrcType()) + "_TO_" +
                GateMetaData::Str(accessor.GetDstType());
        }
        default:
            return "";
    }
    return "";
}

void Gate::PrintWithBytecode(std::string_view comment) const
{
    PrintGateWithAdditionOp(GetBytecodeStr(), comment);
}

void Gate::PrintGateWithAdditionOp(std::string additionOp, std::string_view comment) const
{
    Print(additionOp, false, -1, comment);
}

MarkCode Gate::GetMark(TimeStamp stamp) const
{
    return (stamp_ == stamp) ? mark_ : MarkCode::NO_MARK;
}

void Gate::SetMark(MarkCode mark, TimeStamp stamp)
{
    stamp_ = stamp;
    mark_ = mark;
}
}  // namespace panda::ecmascript::kungfu
