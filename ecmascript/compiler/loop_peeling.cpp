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

#include "ecmascript/compiler/loop_peeling.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/type.h"

namespace panda::ecmascript::kungfu {
void LoopPeeling::CopyLoopExit()
{
    for (auto exit : loopInfo_->loopExits) {
        ASSERT(acc_.GetOpCode(exit) == OpCode::LOOP_EXIT);
        GateRef numIns = 2; // 2: num ins
        GateRef copyExit = GetCopy(acc_.GetState(exit));
        GateRef merge = circuit_->NewGate(circuit_->Merge(numIns), {exit, copyExit});
        auto exitUse = acc_.Uses(exit);
        for (auto it = exitUse.begin(); it != exitUse.end();) {
            if (acc_.GetOpCode(*it) == OpCode::LOOP_EXIT_DEPEND) {
                GateRef depend = *it;
                GateRef copyDepend = GetCopy(acc_.GetDep(depend));
                GateRef selector = circuit_->NewGate(circuit_->DependSelector(numIns), {merge, depend, copyDepend});
                acc_.UpdateAllUses(depend, selector);
                acc_.ReplaceIn(selector, 1, depend);    // 0: index of exit depend
                ++it;
            } else if (acc_.GetOpCode(*it) == OpCode::LOOP_EXIT_VALUE) {
                GateRef value = *it;
                GateRef copyValue = GetCopy(acc_.GetValueIn(value));
                ASSERT(acc_.GetMachineType(value) == acc_.GetMachineType(copyValue));
                GateRef selector = circuit_->NewGate(circuit_->ValueSelector(numIns), acc_.GetMachineType(value),
                                                     {merge, value, copyValue}, acc_.GetGateType(value));
                acc_.UpdateAllUses(value, selector);
                acc_.ReplaceIn(selector, 1, value);    // 0: index of exit depend
                ++it;
            } else if ((*it) == merge) {
                ++it;
            } else {
                it = acc_.ReplaceIn(it, merge);
            }
        }
    }
}

void LoopPeeling::CopyLoopBody()
{
    for (auto gate : loopInfo_->loopBodys) {
        if ((gate == loopInfo_->loopHead) ||
            (acc_.IsSelector(gate) && acc_.GetState(gate) == loopInfo_->loopHead)) {
            continue;
        }
        GateRef copy = GetCopy(gate);
        size_t numIns = acc_.GetNumIns(gate);
        for (size_t i = 0; i < numIns; ++i) {
            GateRef in = acc_.GetIn(gate, i);
            GateRef copyIn = TryGetCopy(in);
            if (copyIn != Circuit::NullGate()) {
                acc_.NewIn(copy, i, copyIn);
            } else {
                acc_.NewIn(copy, i, in);
            }
        }
    }
}

GateRef LoopPeeling::CopySelector(GateRef stateMerge, GateRef selector, size_t numLoopbacks)
{
    GateRef newGate = Circuit::NullGate();
    auto inList = std::vector<GateRef>(1 + numLoopbacks, Circuit::NullGate()); // 1: state
    if (acc_.IsValueSelector(selector)) {
        newGate = circuit_->NewGate(circuit_->ValueSelector(numLoopbacks),
            MachineType::I64, inList.size(), inList.data(), GateType::AnyType());
    } else {
        newGate = circuit_->NewGate(circuit_->DependSelector(numLoopbacks), inList);
    }
    acc_.NewIn(newGate, 0, stateMerge); // 0: is state
    auto numOfIns = acc_.GetNumIns(selector);
    const size_t skipValue = 2; // 2: state & entry value
    ASSERT(numOfIns == numLoopbacks + skipValue);
    for (size_t i = skipValue; i < numOfIns; i++) {
        auto input = acc_.GetIn(selector, i);
        acc_.NewIn(newGate, i - 1, GetCopy(input)); // 1: is state
    }
    return newGate;
}

void LoopPeeling::CopyLoopHeader()
{
    auto numLoopbacks = loopInfo_->loopBacks.size();
    if (numLoopbacks > 1) {
        const size_t skipValue = 1; // 1: entry state
        auto numOfIns = acc_.GetNumIns(loopInfo_->loopHead);
        ASSERT(numOfIns == numLoopbacks + skipValue);
        std::vector<GateRef> inList(numLoopbacks, Circuit::NullGate());
        auto merge = circuit_->NewGate(circuit_->Merge(numLoopbacks), inList);
        for (size_t i = skipValue; i < numOfIns; i++) { // 1: skip entry
            auto input = acc_.GetIn(loopInfo_->loopHead, i);
            acc_.NewIn(merge, i - skipValue, GetCopy(input));
        }

        auto use = acc_.Uses(loopInfo_->loopHead);
        for (auto it = use.begin(); it != use.end(); ++it) {
            if (acc_.IsSelector(*it)) {
                auto selector = CopySelector(merge, *it, numLoopbacks);
                acc_.ReplaceIn(*it, 1, selector);
            }
        }
        acc_.ReplaceIn(loopInfo_->loopHead, 0, merge);  // 0: index of state forward
    } else {
        // replace origin forwards with peeled loop back.
        GateRef stateBack = acc_.GetState(loopInfo_->loopHead, 1); // 1: index of state back
        acc_.ReplaceIn(loopInfo_->loopHead, 0, GetCopy(stateBack));  // 0: index of state forward
        auto use = acc_.Uses(loopInfo_->loopHead);
        for (auto it = use.begin(); it != use.end(); ++it) {
            if (acc_.IsSelector(*it)) {
                GateRef backward = acc_.GetIn(*it, 2);  // 2: index of depend or value back
                acc_.ReplaceIn(*it, 1, GetCopy(backward)); // 1: index of depend or value forward
            }
        }
    }
}

void LoopPeeling::Peel()
{
    SetCopy(loopInfo_->loopHead);
    for (auto gate : loopInfo_->loopBodys) {
        SetCopy(gate);
    }
    for (auto gate : loopInfo_->loopBacks) {
        copies_[gate] = GetCopy(acc_.GetState(gate));
    }
    CopyLoopBody();
    CopyLoopHeader();
    CopyLoopExit();

    if (bcBuilder_) {
        auto asyncList = bcBuilder_->GetAsyncRelatedGates();
        ChunkVector<GateRef> list(chunk_);
        for (auto gate : asyncList) {
            auto copyAsync = TryGetCopy(gate);
            if (copyAsync == Circuit::NullGate()) {
                list.emplace_back(copyAsync);
            }
        }
        for (auto gate : asyncList) {
            bcBuilder_->UpdateAsyncRelatedGate(gate);
        }
    }

    Print();
}

void LoopPeeling::SetCopy(GateRef gate)
{
    ASSERT(copies_.count(gate) == 0);
    if (gate == loopInfo_->loopHead) {
        // copy of head is forward
        copies_[gate] = acc_.GetState(gate);
        return;
    }
    if (acc_.IsSelector(gate) && acc_.GetState(gate) == loopInfo_->loopHead) {
        // copy of head is forward
        copies_[gate] = acc_.GetIn(gate, 1); // 1: index of forward
        return;
    }

    std::vector<GateRef> inList(acc_.GetNumIns(gate), Circuit::NullGate());
    GateRef newGate = circuit_->NewGate(acc_.GetMetaData(gate), inList);
    acc_.SetGateType(newGate, acc_.GetGateType(gate));
    acc_.SetMachineType(newGate, acc_.GetMachineType(gate));
    copies_[gate] = newGate;
    if (acc_.GetOpCode(gate) == OpCode::JS_BYTECODE) {
        bcBuilder_->UpdateBcIndexGate(newGate, bcBuilder_->GetBcIndexByGate(gate));
    }
}

GateRef LoopPeeling::GetCopy(GateRef gate) const
{
    if (copies_.count(gate)) {
        return copies_.at(gate);
    }
    return gate;
}

GateRef LoopPeeling::TryGetCopy(GateRef gate) const
{
    if (copies_.count(gate) > 0) {
        return copies_.at(gate);
    }
    return Circuit::NullGate();
}

void LoopPeeling::Print() const
{
    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "";
        LOG_COMPILER(INFO) << "\033[34m"
                           << "===================="
                           << " After loop peeling "
                           << "[" << GetMethodName() << "]"
                           << "===================="
                           << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "========================= End ==========================" << "\033[0m";
    }
}
}  // namespace panda::ecmascript::kungfu