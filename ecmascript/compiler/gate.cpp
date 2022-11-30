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

namespace panda::ecmascript::kungfu {
std::optional<std::pair<std::string, size_t>> Gate::CheckNullInput() const
{
    const auto numIns = GetNumIns();
    for (size_t idx = 0; idx < numIns; idx++) {
        if (IsInGateNull(idx)) {
            return std::make_pair("In list contains null", idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckStateInput() const
{
    size_t stateStart = 0;
    size_t stateEnd = GetStateCount();
    for (size_t idx = stateStart; idx < stateEnd; idx++) {
        auto stateProp = meta_->GetProperties().statesIn;
        ASSERT(stateProp.has_value());
        auto expectedIn = meta_->GetInStateCode(idx);
        auto actualIn = GetInGateConst(idx)->meta_;
        if (expectedIn == OpCode::NOP) {  // general
            if (!actualIn->IsGeneralState()) {
                return std::make_pair(
                    "State input does not match (expected:<General State> actual:" + actualIn->Str() + ")", idx);
            }
        } else {
            if (expectedIn != actualIn->GetOpCode()) {
                return std::make_pair(
                    "State input does not match (expected:" +
                        GateMetaData::Str(expectedIn) + " actual:" + actualIn->Str() + ")", idx);
            }
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckValueInput(bool isArch64) const
{
    size_t valueStart = GetInValueStarts();
    size_t valueEnd = valueStart + GetInValueCount();
    for (size_t idx = valueStart; idx < valueEnd; idx++) {
        auto expectedIn = meta_->GetInMachineType(idx);
        auto actualIn = GetInGateConst(idx)->meta_->GetProperties().returnValue;
        if (expectedIn == MachineType::FLEX) {
            expectedIn = GetMachineType();
        }
        if (actualIn == MachineType::FLEX) {
            actualIn = GetInGateConst(idx)->GetMachineType();
        }
        if (actualIn == MachineType::ARCH) {
            actualIn = isArch64 ? MachineType::I64 : MachineType::I32;
        }

        if ((expectedIn != actualIn) && (expectedIn != ANYVALUE)) {
            return std::make_pair("Value input does not match (expected: " + MachineTypeToStr(expectedIn) +
                    " actual: " + MachineTypeToStr(actualIn) + ")",
                idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckDependInput() const
{
    size_t dependStart = GetStateCount();
    size_t dependEnd = dependStart + GetDependCount();
    for (size_t idx = dependStart; idx < dependEnd; idx++) {
        if (GetInGateConst(idx)->GetDependCount() == 0 &&
            GetInGateConst(idx)->GetOpCode() != OpCode::DEPEND_ENTRY) {
            return std::make_pair("Depend input is side-effect free", idx);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckStateOutput() const
{
    if (GetMetaData()->IsState()) {
        size_t cnt = 0;
        const Gate *curGate = this;
        if (!curGate->IsFirstOutNull()) {
            const Out *curOut = curGate->GetFirstOutConst();
            if (curOut->IsStateEdge() && curOut->GetGateConst()->GetMetaData()->IsState()) {
                cnt++;
            }
            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                if (curOut->IsStateEdge() && curOut->GetGateConst()->GetMetaData()->IsState()) {
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
            return std::make_pair("Number of state out branches is not valid (expected:" + std::to_string(expected) +
                    " actual:" + std::to_string(cnt) + ")",
                -1);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckBranchOutput() const
{
    std::map<std::pair<OpCode, BitField>, size_t> setOfOps;
    if (GetOpCode() == OpCode::IF_BRANCH || GetOpCode() == OpCode::SWITCH_BRANCH) {
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
            return std::make_pair("Duplicate state out branches", -1);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckNOP() const
{
    if (GetOpCode() == OpCode::NOP) {
        if (!IsFirstOutNull()) {
            return std::make_pair("NOP gate used by other gates", -1);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckSelector() const
{
    if (GetOpCode() == OpCode::VALUE_SELECTOR || GetOpCode() == OpCode::DEPEND_SELECTOR) {
        auto stateOp = GetInGateConst(0)->GetOpCode();
        if (stateOp == OpCode::MERGE || stateOp == OpCode::LOOP_BEGIN) {
            if (GetInGateConst(0)->GetNumIns() != GetNumIns() - 1) {
                if (GetOpCode() == OpCode::DEPEND_SELECTOR) {
                    return std::make_pair("Number of depend flows does not match control flows (expected:" +
                            std::to_string(GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(GetNumIns() - 1) + ")",
                        -1);
                } else {
                    return std::make_pair("Number of data flows does not match control flows (expected:" +
                            std::to_string(GetInGateConst(0)->GetNumIns()) +
                            " actual:" + std::to_string(GetNumIns() - 1) + ")",
                        -1);
                }
            }
        } else {
            return std::make_pair(
                "State input does not match (expected:[MERGE|LOOP_BEGIN] actual:" +
                GateMetaData::Str(stateOp) + ")", 0);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::CheckRelay() const
{
    if (GetOpCode() == OpCode::DEPEND_RELAY) {
        auto stateOp = GetInGateConst(0)->GetOpCode();
        if (!(stateOp == OpCode::IF_TRUE || stateOp == OpCode::IF_FALSE || stateOp == OpCode::SWITCH_CASE ||
            stateOp == OpCode::DEFAULT_CASE || stateOp == OpCode::IF_SUCCESS || stateOp == OpCode::IF_EXCEPTION ||
            stateOp == OpCode::ORDINARY_BLOCK)) {
            return std::make_pair("State input does not match ("
                "expected:[IF_TRUE|IF_FALSE|SWITCH_CASE|DEFAULT_CASE|IF_SUCCESS|IF_EXCEPTION|ORDINARY_BLOCK] actual:" +
                 GateMetaData::Str(stateOp) + ")", 0);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<std::string, size_t>> Gate::SpecialCheck() const
{
    {
        auto ret = CheckNOP();
        if (ret.has_value()) {
            return ret;
        }
    }
    {
        auto ret = CheckSelector();
        if (ret.has_value()) {
            return ret;
        }
    }
    {
        auto ret = CheckRelay();
        if (ret.has_value()) {
            return ret;
        }
    }
    return std::nullopt;
}

bool Gate::Verify(bool isArch64) const
{
    std::string errorString;
    size_t highlightIdx = -1;
    bool failed = false;
    {
        auto ret = CheckNullInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckStateInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckValueInput(isArch64);
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckDependInput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckStateOutput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = CheckBranchOutput();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (!failed) {
        auto ret = SpecialCheck();
        if (ret.has_value()) {
            failed = true;
            std::tie(errorString, highlightIdx) = ret.value();
        }
    }
    if (failed) {
        LOG_COMPILER(ERROR) << "[Verifier][Error] Gate level input list schema verify failed";
        Print("", true, highlightIdx);
        LOG_COMPILER(ERROR) << "Note: " << errorString;
    }
    return !failed;
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
    return GetStateCount() + GetDependCount();
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
    GlobalTSTypeRef r = gateType.GetGTRef();
    uint32_t m = r.GetModuleId();
    uint32_t l = r.GetLocalId();
    return name + std::string("-GT(M=") + std::to_string(m) +
           std::string(", L=") + std::to_string(l) + std::string(")");
}

void Gate::Print(std::string bytecode, bool inListPreview, size_t highlightIdx) const
{
    auto opcode = GetOpCode();
    if (opcode != OpCode::NOP) {
        std::string log("{\"id\":" + std::to_string(id_) + ", \"op\":\"" + GateMetaData::Str(opcode) + "\", ");
        log += ((bytecode.compare("") == 0) ? "" : "\"bytecode\":\"") + bytecode;
        log += ((bytecode.compare("") == 0) ? "" : "\", ");
        log += "\"MType\":\"" + MachineTypeStr(GetMachineType()) + ", ";
        log += "bitfield=" + std::to_string(GetBitField()) + ", ";
        log += "type=" + GateTypeStr(type_) + ", ";
        log += "stamp=" + std::to_string(static_cast<uint32_t>(stamp_)) + ", ";
        log += "mark=" + std::to_string(static_cast<uint32_t>(mark_)) + ", ";
        log += "\",\"in\":[";

        size_t idx = 0;
        auto stateSize = GetStateCount();
        auto dependSize = GetDependCount();
        auto valueSize = GetInValueCount();
        auto rootSize = GetRootCount();
        idx = PrintInGate(stateSize, idx, 0, inListPreview, highlightIdx, log);
        idx = PrintInGate(stateSize + dependSize, idx, stateSize, inListPreview, highlightIdx, log);
        idx = PrintInGate(stateSize + dependSize + valueSize, idx, stateSize + dependSize,
                          inListPreview, highlightIdx, log);
        PrintInGate(stateSize + dependSize + valueSize + rootSize, idx, stateSize + dependSize + valueSize,
                    inListPreview, highlightIdx, log, true);

        log += "], \"out\":[";

        if (!IsFirstOutNull()) {
            const Out *curOut = GetFirstOutConst();
            opcode = curOut->GetGateConst()->GetOpCode();
            log += std::to_string(curOut->GetGateConst()->GetId()) +
                    (inListPreview ? std::string(":" + GateMetaData::Str(opcode)) : std::string(""));

            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                log += ", " +  std::to_string(curOut->GetGateConst()->GetId()) +
                       (inListPreview ? std::string(":" + GateMetaData::Str(opcode))
                                       : std::string(""));
            }
        }
        log += "]},";
        LOG_COMPILER(INFO) << std::dec << log;
    }
}

void Gate::ShortPrint(std::string bytecode, bool inListPreview, size_t highlightIdx) const
{
    auto opcode = GetOpCode();
    if (opcode != OpCode::NOP) {
        std::string log("(\"id\"=" + std::to_string(id_) + ", \"op\"=\"" + GateMetaData::Str(opcode) + "\", ");
        log += ((bytecode.compare("") == 0) ? "" : "bytecode=") + bytecode;
        log += ((bytecode.compare("") == 0) ? "" : ", ");
        log += "\"MType\"=\"" + MachineTypeStr(GetMachineType()) + ", ";
        log += "bitfield=" + std::to_string(GetBitField()) + ", ";
        log += "type=" + GateTypeStr(type_) + ", ";
        log += "\", in=[";

        size_t idx = 0;
        auto stateSize = GetStateCount();
        auto dependSize = GetDependCount();
        auto valueSize = GetInValueCount();
        auto rootSize = GetRootCount();
        idx = PrintInGate(stateSize, idx, 0, inListPreview, highlightIdx, log);
        idx = PrintInGate(stateSize + dependSize, idx, stateSize, inListPreview, highlightIdx, log);
        idx = PrintInGate(stateSize + dependSize + valueSize, idx, stateSize + dependSize,
                          inListPreview, highlightIdx, log);
        PrintInGate(stateSize + dependSize + valueSize + rootSize, idx, stateSize + dependSize + valueSize,
                    inListPreview, highlightIdx, log, true);

        log += "], out=[";

        if (!IsFirstOutNull()) {
            const Out *curOut = GetFirstOutConst();
            opcode = curOut->GetGateConst()->GetOpCode();
            log += std::to_string(curOut->GetGateConst()->GetId()) +
                   (inListPreview ? std::string(":" + GateMetaData::Str(opcode)) : std::string(""));

            while (!curOut->IsNextOutNull()) {
                curOut = curOut->GetNextOutConst();
                log += ", " +  std::to_string(curOut->GetGateConst()->GetId()) +
                       (inListPreview ? std::string(":" + GateMetaData::Str(opcode))
                                      : std::string(""));
            }
        }
        log += "])";
        LOG_COMPILER(INFO) << std::dec << log;
    }
}

size_t Gate::PrintInGate(size_t numIns, size_t idx, size_t size, bool inListPreview, size_t highlightIdx,
                         std::string &log, bool isEnd) const
{
    log += "[";
    for (; idx < numIns; idx++) {
        log += ((idx == size) ? "" : ", ");
        log += ((idx == highlightIdx) ? "\033[4;31m" : "");
        log += ((IsInGateNull(idx)
                ? "N"
                : (std::to_string(GetInGateConst(idx)->GetId()) +
                    (inListPreview ? std::string(":" + GateMetaData::Str(GetInGateConst(idx)->GetOpCode()))
                                   : std::string("")))));
        log += ((idx == highlightIdx) ? "\033[0m" : "");
    }
    log += "]";
    log += ((isEnd) ? "" : ", ");
    return idx;
}

void Gate::PrintByteCode(std::string bytecode) const
{
    Print(bytecode);
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
